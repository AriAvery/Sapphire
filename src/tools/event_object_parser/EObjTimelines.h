#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace xiv::dat { class GameData; }
namespace Sapphire::Data { class ExdData; }

namespace EObjTimelines
{
  struct Bindings
  {
    uint8_t NamePlateInstanceID;
    uint8_t TimelineShowingID;
    uint8_t TimelineHidingID;
    uint8_t TimelineShownID;
    uint8_t TimelineHiddenID;
    uint8_t GeneralPurposeTimelineIDs[ 16 ];
  };
  static_assert( sizeof( Bindings ) == 21 );

  struct Mapping
  {
    Bindings bindings{};
    std::map< uint32_t, std::string > names;
  };

  struct Placement
  {
    uint32_t baseId;
    uint32_t instanceId;
    uint32_t boundId;
    std::string name;
    std::string layer;
    std::string path;
    std::string status;
  };

  struct Zone
  {
    std::vector< Placement > objects;
    std::vector< std::string > errors;
  };

  Mapping readMapping( const std::vector< char >& data );
  std::string exportedPath( Sapphire::Data::ExdData& exd, uint32_t baseId );
  Zone readZone( xiv::dat::GameData& game, Sapphire::Data::ExdData& exd, const std::string& lvb );
}
