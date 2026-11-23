#include "EObjTimelines.h"

#include <Exd/ExdData.h>
#include <datReader/DatCategories/bg/sgb.h>
#include <cstring>
#include <stdexcept>

namespace
{
  struct Layout
  {
    const std::vector< char >& bytes;

    template< typename T >
    T read( size_t at ) const
    {
      if( at > bytes.size() || bytes.size() - at < sizeof( T ) )
        throw std::runtime_error( "Truncated layout record" );
      T value;
      std::memcpy( &value, bytes.data() + at, sizeof( T ) );
      return value;
    }

    size_t relative( size_t base, int32_t offset ) const
    {
      const auto at = static_cast< int64_t >( base ) + offset;
      if( at < 0 || static_cast< uint64_t >( at ) > bytes.size() )
        throw std::runtime_error( "Invalid layout offset" );
      return static_cast< size_t >( at );
    }

    size_t table( size_t base, int32_t offset, int32_t count, size_t stride ) const
    {
      if( !count )
        return base;
      const auto at = relative( base, offset );
      if( count < 0 || static_cast< size_t >( count ) > ( bytes.size() - at ) / stride )
        throw std::runtime_error( "Invalid layout table" );
      return at;
    }

    std::string stringAt( size_t base, int32_t offset ) const
    {
      if( !offset )
        return {};
      const auto at = relative( base, offset );
      const char* start = bytes.data() + at;
      const auto end = static_cast< const char* >( std::memchr( start, 0, bytes.size() - at ) );
      if( !end )
        throw std::runtime_error( "Unterminated layout string" );
      return std::string( start, end );
    }

    size_t chunk( const char* magic, const char* type ) const
    {
      auto header = read< SGB_HEADER >( 0 );
      const bool extended = std::memcmp( magic, "LGB1", 4 ) == 0 && read< uint32_t >( 12 ) == 1;
      auto at = extended ? 32u : 12u;
      if( std::memcmp( header.magic, magic, 4 ) || at + 8 > bytes.size() ||
          std::memcmp( bytes.data() + at, type, 4 ) )
        throw std::runtime_error( "Unexpected layout format" );
      return at + ( extended ? 16 : 8 );
    }
  };

  using Groups = std::map< uint32_t, std::string >;

  void collectGroup( const Layout& layout, size_t base, const std::string& file,
                     std::vector< EObjTimelines::Placement >& objects, Groups& groups )
  {
    auto group = layout.read< LayerGroup >( base );
    const auto layers = layout.table( base, group.Layers, group.Layer_Count, 4 );
    for( int i = 0; i < group.Layer_Count; ++i )
    {
      const auto layer = layout.relative( layers, layout.read< int32_t >( layers + i * 4 ) );
      const auto name = layout.stringAt( layer, layout.read< int32_t >( layer + 4 ) );
      const auto count = layout.read< int32_t >( layer + 12 );
      const auto entries = layout.table( layer, layout.read< int32_t >( layer + 8 ), count, 4 );
      for( int j = 0; j < count; ++j )
      {
        const auto at = layout.relative( entries, layout.read< int32_t >( entries + j * 4 ) );
        const auto type = layout.read< eAssetType >( at );
        if( type == eAssetType::SharedGroup )
        {
          auto sg = layout.read< SGData >( at );
          auto path = layout.stringAt( at, sg.AssetPath );
          auto [it, added] = groups.emplace( sg.InstanceID, path );
          if( !added && it->second != path )
            it->second.clear();
        }
        else if( type == eAssetType::EventObject )
        {
          auto obj = layout.read< EObjData >( at );
          objects.push_back( { obj.BaseId, obj.InstanceID, obj.BoundInstanceID,
                               layout.stringAt( at, obj.Name ), file + " / " + name, {}, {} } );
        }
      }
    }
  }
}

EObjTimelines::Mapping EObjTimelines::readMapping( const std::vector< char >& data )
{
  Layout layout{ data };
  const auto base = layout.chunk( "SGB1", "SCN1" );
  auto scene = layout.read< Scene_t >( base );
  Mapping result;
  if( scene.SGSettings )
    result.bindings = layout.read< Bindings >( layout.relative( base, scene.SGSettings ) );
  if( scene.SGTimelineFolder )
  {
    const auto at = layout.relative( base, scene.SGTimelineFolder );
    const auto folder = layout.read< SGTimelineFolder_t >( at );
    const auto entries = layout.table( at, folder.SGTimelines, folder.SGTimeline_Count, sizeof( SGTimeline_t ) );
    for( int i = 0; i < folder.SGTimeline_Count; ++i )
    {
      const auto entry = entries + i * sizeof( SGTimeline_t );
      auto timeline = layout.read< SGTimeline_t >( entry );
      result.names.emplace( timeline.MemberID, layout.stringAt( entry, timeline.Name ) );
    }
  }
  return result;
}

std::string EObjTimelines::exportedPath( Sapphire::Data::ExdData& exd, uint32_t baseId )
{
  auto obj = exd.getRow< Excel::EObj >( baseId );
  if( !obj )
    throw std::runtime_error( "Missing EObj row " + std::to_string( baseId ) );
  if( !obj->data().SharedGroup )
    return {};
  auto sg = exd.getRow< Excel::ExportedSG >( obj->data().SharedGroup );
  if( !sg )
    throw std::runtime_error( "Missing ExportedSG " + std::to_string( obj->data().SharedGroup ) );
  auto path = sg->getString( sg->data().SGFilePath );
  if( path.empty() )
    throw std::runtime_error( "ExportedSG has no asset path" );
  return path;
}

EObjTimelines::Zone EObjTimelines::readZone( xiv::dat::GameData& game, Sapphire::Data::ExdData& exd,
                                             const std::string& lvb )
{
  Zone zone;
  if( lvb.empty() )
  {
    zone.errors.emplace_back( "This territory has no layout." );
    return zone;
  }
  Groups groups;
  const auto path = "bg/" + lvb + ".lvb";
  auto file = game.getFile( path );
  Layout layout{ file->access_data_sections().at( 0 ) };
  const auto base = layout.chunk( "LVB1", "SCN1" );
  const auto scene = layout.read< Scene_t >( base );
  const auto embedded = layout.table( base, scene.LayerGroups, scene.LayerGroup_Count, sizeof( LayerGroup ) );
  for( int i = 0; i < scene.LayerGroup_Count; ++i )
    collectGroup( layout, embedded + i * sizeof( LayerGroup ), path, zone.objects, groups );

  const auto paths = layout.table( base, scene.LGBAssetPaths, scene.LGBAssetPath_Count, 4 );
  for( int i = 0; i < scene.LGBAssetPath_Count; ++i )
  {
    const auto layerPath = layout.stringAt( paths, layout.read< int32_t >( paths + i * 4 ) );
    try
    {
      auto layerFile = game.getFile( layerPath );
      Layout layer{ layerFile->access_data_sections().at( 0 ) };
      collectGroup( layer, layer.chunk( "LGB1", "LGP1" ), layerPath, zone.objects, groups );
    }
    catch( const std::exception& error )
    {
      zone.errors.push_back( layerPath + ": " + error.what() );
    }
  }

  for( auto& obj : zone.objects )
  {
    try
    {
      auto row = exd.getRow< Excel::EObj >( obj.baseId );
      if( row )
      {
        auto name = row->getString( row->data().Text.SGL );
        if( !name.empty() )
          obj.name = name;
      }
      if( obj.boundId )
      {
        auto it = groups.find( obj.boundId );
        if( it != groups.end() )
          obj.path = it->second;
        if( obj.path.empty() )
          obj.status = "Bound placement could not be resolved to a shared group.";
      }
      else
      {
        obj.path = exportedPath( exd, obj.baseId );
        if( obj.path.empty() )
          obj.status = "No shared group assigned.";
      }
    }
    catch( const std::exception& error )
    {
      obj.status = error.what();
    }
  }
  return zone;
}
