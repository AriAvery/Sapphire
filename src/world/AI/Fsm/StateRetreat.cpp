#include "StateRetreat.h"
#include "Actor/BNpc.h"
#include "AI/Controller/Controller.h"
#include <Service.h>
#include <Manager/TerritoryMgr.h>

#include <Territory/Territory.h>
#include <Navi/NaviProvider.h>

using namespace Sapphire::World;

void AI::Fsm::StateRetreat::onUpdate( Entity::GameObjectPtr& pEntity, uint64_t tickCount )
{
  if( auto pBNpc = pEntity->getAsBNpc() )
  {
    auto& bnpc = *pBNpc;

    if( bnpc.moveTo( bnpc.getSpawnPos() ) )
    {
      bnpc.setRoamTargetReached( true );
      bnpc.setLastRoamTargetReachedTime( Common::Util::getTimeSeconds() );
    }
  }
}

void AI::Fsm::StateRetreat::onEnter( Entity::GameObjectPtr& pEntity )
{
  auto& teriMgr = Common::Service< World::Manager::TerritoryMgr >::ref();
  auto pZone = teriMgr.getTerritoryByGuId( pEntity->getTerritoryId() );
  auto pNaviProvider = pZone->getNaviProvider();

  if( auto pBNpc = pEntity->getAsBNpc() )
  {
    auto& bnpc = *pBNpc;

    bnpc.setState( Entity::BNpcState::Retreat );
    bnpc.setRoamTargetReached( false );
    bnpc.setInvincibilityType( Common::InvincibilityType::InvincibilityIgnoreDamage );

    if( pNaviProvider && bnpc.getAgentId() != -1 )
    {
      pNaviProvider->updateAgentParameters( bnpc.getAgentId(), bnpc.getRadius(), true, bnpc.getRunSpeed(), true, true );
      pNaviProvider->setMoveTarget( bnpc.getAgentId(), bnpc.getSpawnPos() );
    }
  }
}

void AI::Fsm::StateRetreat::onExit( Entity::GameObjectPtr& pEntity )
{
  if( auto pBNpc = pEntity->getAsBNpc() )
  {
    auto& bnpc = *pBNpc;

    bnpc.setState( Entity::BNpcState::Idle );
    auto& teriMgr = Common::Service< World::Manager::TerritoryMgr >::ref();
    auto pZone = teriMgr.getTerritoryByGuId( bnpc.getTerritoryId() );
    if( pZone && pZone->getNaviProvider() && bnpc.getAgentId() != -1 )
    {
      auto flags = bnpc.getController()->getPath().m_flags;
      pZone->getNaviProvider()->updateAgentParameters( bnpc.getAgentId(), bnpc.getRadius(), false, bnpc.getWalkSpeed(),
                                                      flags & Controller::Controller::IgnoreActorCollision,
                                                      flags & Controller::Controller::IgnoreObstacles );
    }

    bnpc.setOwner( nullptr );
    bnpc.setRoamTargetReached( false );
    bnpc.setInvincibilityType( Common::InvincibilityType::InvincibilityNone );
    bnpc.setRot( bnpc.getSpawnRot() );
    if( bnpc.getHpPercent() < 100 )
      bnpc.heal( bnpc.getMaxHp() );
  }
}

