#include <ScriptObject.h>

#include <Actor/BNpc.h>
#include <Encounter/Encounter.h>
#include <Encounter/TimelinePack.h>

#include <vector>

using namespace Sapphire;
using namespace Sapphire::World::Encounter;

class TamTaraReinforcements : public ScriptAPI::MechanicScript
{
public:
  void begin( const nlohmann::json& args, TimelinePack& pack, EncounterPtr pEncounter )
  {
    for( const auto& wave : args.at( "waves" ) )
    {
      m_waves.push_back( { wave.at( "hpPercent" ).get< uint32_t >(),
                           wave.at( "imps" ).get< std::vector< uint32_t > >(),
                           wave.at( "guards" ).get< std::vector< uint32_t > >() } );
    }
  }

  void update( uint64_t tick, TimelinePack& pack, EncounterPtr pEncounter ) override
  {
    auto boss = pack.getBNpcByRef( "Galvanth", pEncounter );
    if( !boss || !boss->isAlive() )
      return;

    if( !m_imps.empty() )
    {
      for( const auto& imp : m_imps )
      {
        if( imp->isAlive() )
          return;
      }
      m_imps.clear();
      boss->setInvincibilityType( Common::InvincibilityNone );
    }

    if( m_nextWave == m_waves.size() )
      return;

    const auto& wave = m_waves[ m_nextWave ];
    if( boss->getHpPercent() > wave.hpPercent )
      return;

    for( auto layoutId : wave.imps )
    {
      if( auto imp = spawn( pEncounter, layoutId, 516, boss->hateListGetHighest() ) )
        m_imps.push_back( imp );
    }
    if( !m_imps.empty() )
      boss->setInvincibilityType( Common::InvincibilityIgnoreDamage );

    for( auto layoutId : wave.guards )
      spawn( pEncounter, layoutId, 172, boss->hateListGetHighest() );

    ++m_nextWave;
  }

private:
  struct Wave
  {
    uint32_t hpPercent;
    std::vector< uint32_t > imps;
    std::vector< uint32_t > guards;
  };

  Entity::BNpcPtr spawn( EncounterPtr pEncounter, uint32_t layoutId, uint32_t hp, Entity::CharaPtr target )
  {
    auto npc = pEncounter->getTeriPtr()->createAnimatedBNpc( layoutId, hp, Common::BNpcType::Enemy );
    if( npc )
    {
      pEncounter->addBNpc( npc );
      if( target )
        npc->aggro( target );
    }
    return npc;
  }

  std::vector< Wave > m_waves;
  std::vector< Entity::BNpcPtr > m_imps;
  size_t m_nextWave{ 0 };
};

EXPOSE_MECHANIC_SCRIPT( TamTaraReinforcements, MECHANIC_FUNCTION( begin ) );
