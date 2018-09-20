#include <Common.h>
#include <Logging/Logger.h>
#include <Util/Util.h>
#include <Util/UtilMath.h>

#include <Network/GamePacketNew.h>
#include <Network/PacketDef/Zone/ServerZoneDef.h>

#include "Actor/Player.h"

#include "Forwards.h"
#include "HousingZone.h"
#include "Framework.h"

extern Core::Framework g_fw;

using namespace Core::Common;
using namespace Core::Network::Packets;
using namespace Core::Network::Packets::Server;

Core::HousingZone::HousingZone( uint8_t wardNum,
                                uint16_t territoryId,
                                uint32_t guId,
                                const std::string& internalName,
                                const std::string& contentName ) :
  Zone( territoryId, guId, internalName, contentName ),
  m_wardNum( wardNum )
{

}

bool Core::HousingZone::init()
{
  uint32_t landSetId;

  for( landSetId = 0; landSetId < 60; landSetId++ )
  {
    //TODO: load house information here
  }

  return true;
}

Core::HousingZone::~HousingZone()
{

}

void Core::HousingZone::onPlayerZoneIn( Entity::Player& player )
{
  auto pLog = g_fw.get< Logger >();
  pLog->debug( "HousingZone::onPlayerZoneIn: Zone#" + std::to_string( getGuId() ) + "|" +
               +", Entity#" + std::to_string( player.getId() ) );

  uint32_t landSetId;
  uint32_t yardPacketNum;
  uint32_t yardPacketTotal = 8;

  auto landsetInitializePacket = makeZonePacket< FFXIVIpcLandsetInitialize >( player.getId() );

  landsetInitializePacket->data().wardNum = m_wardNum;
  landsetInitializePacket->data().zoneId = player.getZoneId();
  //TODO: get current WorldId
  landsetInitializePacket->data().worldId = 67;
  //TODO: handle Subdivision
  landsetInitializePacket->data().subInstance = 1;

  for( landSetId = 0; landSetId < 30; landSetId++ )
  {
    landsetInitializePacket->data().landset[ landSetId ].houseSize = 1;
    landsetInitializePacket->data().landset[ landSetId ].houseState = 1;
  }

  player.queuePacket( landsetInitializePacket );

  for( yardPacketNum = 0; yardPacketNum < yardPacketTotal; yardPacketNum++ )
  {
    auto landsetYardInitializePacket = makeZonePacket< FFXIVIpcLandsetYardInitialize >( player.getId() );
    landsetYardInitializePacket->data().unknown1 = 0xFFFFFFFF;
    landsetYardInitializePacket->data().unknown2 = 0xFFFFFFFF;
    landsetYardInitializePacket->data().unknown3 = 0xFF;
    landsetYardInitializePacket->data().packetNum = yardPacketNum;
    landsetYardInitializePacket->data().packetTotal = yardPacketTotal;

    //TODO: Add Objects here

    player.queuePacket( landsetYardInitializePacket );
  }

}

uint8_t Core::HousingZone::getWardNum() const
{
  return m_wardNum;
}
