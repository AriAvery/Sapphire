#ifndef _LGB_H
#define _LGB_H

#include <cstring>
#include <memory>
#include <cstdint>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <functional>

#include "matrix4.h"
#include "vec3.h"
#include "sgb.h"
#include "DatCategories/bg/LgbTypes.h"
#include "DatCategories/Layer.h"

// based on https://github.com/ufx/SaintCoinach/blob/master/SaintCoinach/Graphics/Lgb/
struct LGB_FILE;
struct LGB_FILE_HEADER;
struct LGB_GROUP;
struct LGB_GROUP_HEADER;

class LgbEntry
{
public:
  char* m_buf;
  uint32_t m_offset;
  InstanceObject header;
  std::string name;
  LgbEntry()
  {
    m_buf = nullptr;
    m_offset = 0;
    header = {};
  };

  LgbEntry( char* buf, size_t offset )
  {
    m_buf = buf;
    m_offset = static_cast< uint32_t >( offset );
    header = *reinterpret_cast< InstanceObject* >( buf + offset );
  };

  const eAssetType getType() const
  {
    return static_cast< eAssetType >( header.AssetType );
  };

  virtual ~LgbEntry()
  {
  };
};


class LGB_BGPARTS_ENTRY : public LgbEntry
{
public:
  BgPartsData data;
  std::string modelFileName;
  std::string collisionFileName;

  LGB_BGPARTS_ENTRY() = default;

  LGB_BGPARTS_ENTRY( char* buf, size_t offset ) : LgbEntry( buf, offset )
  {
    data = *reinterpret_cast< BgPartsData* >( buf + offset );
    name = std::string( buf + offset + header.Name );
    modelFileName = std::string( buf + offset + data.AssetPath );
    collisionFileName = std::string( buf + offset + data.CollisionAssetPath );
  };
};






struct LGB_GROUP
{
  using AssetTypeFilter = std::function< bool( eAssetType ) >;

  LGB_FILE* parent;
  Layer header;
  LayerSetReferencedList layerSetReferencedList;
  std::string name;
  std::vector< std::shared_ptr< InstanceObjectEntry > > entries;
  std::vector< LayerSetReferenced > refs;

  LGB_GROUP( char* buf, const LayoutSection& section, LGB_FILE* parentStruct, size_t offset,
             bool extended, const AssetTypeFilter* pTypeFilter ) : parent( parentStruct ), header{}, layerSetReferencedList{}
  {
    if( extended )
    {
      // Extended layers share the prefix through FestivalPhaseID, followed by the OBSet reference table.
      section.read< uint32_t >( offset + 32 );
      memcpy( &header, buf + offset, offsetof( Layer, IsTemporary ) );
      header.OBSetReferencedList = section.read< int32_t >( offset + 28 );
      header.OBSetReferencedList_Count = section.read< int32_t >( offset + 32 );
    }
    else
      header = section.read< Layer >( offset );

    name = section.stringAt( offset, header.Name );
    if( header.LayerSetRef )
    {
      const auto ref = section.relative( offset, header.LayerSetRef );
      layerSetReferencedList = section.read< LayerSetReferencedList >( ref );
      const auto count = layerSetReferencedList.LayerSetCount;
      const auto table = section.table( ref, layerSetReferencedList.LayerSets, count, sizeof( LayerSetReferenced ) );
      refs.resize( count );
      if( count )
        memcpy( refs.data(), buf + table, count * sizeof( LayerSetReferenced ) );
    }

    const auto entriesOffset = section.table( offset, header.InstanceObjects, header.InstanceObject_Count, 4 );
    entries.reserve( header.InstanceObject_Count );
    for( int i = 0; i < header.InstanceObject_Count; ++i )
    {
      const auto entryOffset = section.relative( entriesOffset, section.read< int32_t >( entriesOffset + i * 4 ) );
      const auto type = section.read< eAssetType >( entryOffset );
      if( pTypeFilter && !( *pTypeFilter )( type ) )
        continue;

      // These two entries are also read independently by the Zone Editor.
      if( type == eAssetType::SharedGroup )
      {
        const auto sg = section.read< SGData >( entryOffset );
        entries.emplace_back( std::make_shared< SharedGroupEntry >( buf, entryOffset, sg,
          section.stringAt( entryOffset, sg.Name ), section.stringAt( entryOffset, sg.AssetPath ) ) );
        continue;
      }
      if( type == eAssetType::EventObject )
      {
        const auto obj = section.read< EObjData >( entryOffset );
        entries.emplace_back( std::make_shared< EventObjectEntry >( buf, entryOffset, obj,
          section.stringAt( entryOffset, obj.Name ) ) );
        continue;
      }
      try
      {
        switch( type )
        {
          case eAssetType::BG:
          {
            entries.emplace_back( std::make_shared< BGEntry >( buf, entryOffset ) );
            break;
          }
          case eAssetType::EventNPC:
          {
            entries.emplace_back( std::make_shared< EventNPCEntry >( buf, entryOffset ) );
            break;
          }
          case eAssetType::ExitRange:
          {
            entries.emplace_back( std::make_shared< ExitRangeEntry >( buf, entryOffset ) );
            break;
          }
          case eAssetType::EventRange:
          {
            entries.emplace_back( std::make_shared< EventRangeEntry >( buf, entryOffset ) );
            break;
          }
          case eAssetType::PopRange:
          {
            entries.emplace_back( std::make_shared< PopRangeEntry >( buf, entryOffset ) );
            break;
          }
          case eAssetType::MapRange:
          {
            entries.emplace_back( std::make_shared< MapRangeEntry >( buf, entryOffset ) );
            break;
          }
          case eAssetType::BattleNPC:
          {
            entries.emplace_back( std::make_shared< BattleNPCEntry >( buf, entryOffset ) );
            break;
          }
          case eAssetType::CollisionBox:
          {
            entries.emplace_back( std::make_shared< CollisionBoxEntry >( buf, entryOffset ) );
            break;
          }
          default:
          {
            entries.emplace_back( std::make_shared< InstanceObjectEntry >( buf, entryOffset ) );
            break;
          }
        }
      }
      catch( std::exception& e )
      {
        std::cout << name << " " << e.what() << std::endl;
      }
    }
  };
};

struct LGB_FILE_HEADER
{
  char magic[4]; // LGB 1
  uint32_t fileSize;
  uint32_t totalChunkCount;
  char magic2[4]; // LGP1
  uint32_t chunkSize;
  uint32_t layerGroupId;
  uint32_t nameOff;
  uint32_t groups;
  int32_t groupCount;
};

struct LGB_FILE
{
  using AssetTypeFilter = std::function< bool( eAssetType ) >;

  LGB_FILE_HEADER header;
  std::vector< LGB_GROUP > groups;
  std::string m_name;

  LGB_FILE( char* buf, const std::string& name, const AssetTypeFilter* pTypeFilter = nullptr ) : LGB_FILE( buf, pTypeFilter )
  {
  }

  LGB_FILE( char* buf, const AssetTypeFilter* pTypeFilter = nullptr ) :
    LGB_FILE( buf, reinterpret_cast< LGB_FILE_HEADER* >( buf )->fileSize, pTypeFilter )
  {
  }

  LGB_FILE( char* buf, size_t size, const AssetTypeFilter* pTypeFilter )
  {
    const LayoutSection section{ buf, size };
    const auto base = section.chunk( "LGB1", "LGP1" );
    header = section.read< LGB_FILE_HEADER >( 0 );
    const auto group = section.read< LayerGroup >( base );
    m_name = section.stringAt( base, group.Name );
    if( base != 20 )
    {
      memcpy( header.magic2, "LGP1", 4 );
      header.chunkSize = section.read< uint32_t >( 36 );
      header.layerGroupId = group.LayerGroupID;
      header.nameOff = group.Name;
      header.groups = group.Layers;
      header.groupCount = group.Layer_Count;
    }
    readLayers( buf, section, base, base != 20, pTypeFilter, groups, this );
  }

  static void readLayers( char* buf, const LayoutSection& section, size_t base, bool extended,
                          const AssetTypeFilter* pTypeFilter, std::vector< LGB_GROUP >& groups,
                          LGB_FILE* parent = nullptr )
  {
    const auto group = section.read< LayerGroup >( base );
    const auto layers = section.table( base, group.Layers, group.Layer_Count, 4 );
    for( int i = 0; i < group.Layer_Count; ++i )
    {
      const auto offset = section.relative( layers, section.read< int32_t >( layers + i * 4 ) );
      groups.emplace_back( buf, section, parent, offset, extended, pTypeFilter );
    }
  }
};

/*
#if __cplusplus >= 201703L
#include <experimental/filesystem>
std::map<std::string, LGB_FILE> getLgbFiles( const std::string& dir )
{
   namespace fs = std::experimental::filesystem;
   std::map<std::string, LGB_FILE> fileMap;
   for( const auto& path : fs::recursive_directory_iterator( dir ) )
   {
      if( path.path().extension() == ".lgb" )
      {
         const auto& strPath = path.path().string();
         auto f = fopen( strPath.c_str(), "rb" );
         fseek( f, 0, SEEK_END );
         const auto size = ftell( f );
         std::vector<char> bytes( size );
         rewind( f );
         fread( bytes.data(), 1, size, f );
         fclose( f );
         try
         {
            LGB_FILE lgbFile( bytes.data() );
            fileMap.insert( std::make_pair( strPath, lgbFile ) );
         }
         catch( std::exception& e )
         {
            std::cout << "Unable to load " << strPath << std::endl;
         }
      }
   }
   return fileMap;
}
#endif
*/
#endif
