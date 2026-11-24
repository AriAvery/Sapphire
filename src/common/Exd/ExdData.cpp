#include "ExdData.h"

using namespace Sapphire;

bool Data::ExdData::init( const std::string& path )
{
  try
  {
    m_data = std::make_shared< xiv::dat::GameData >( path );

    if( !m_data )
    {
      return false;
    }

    m_exd_data = std::make_shared< xiv::exd::ExdData >( *m_data );

    if( !m_exd_data )
    {
      return false;
    }
  }
  catch( std::runtime_error& e )
  {
    return false;
  }

  return true;
}

std::string Data::ExdData::getEObjSharedGroupPath( uint32_t baseId )
{
  auto obj = getRow< Excel::EObj >( baseId );
  if( !obj )
    throw std::runtime_error( "Missing EObj row " + std::to_string( baseId ) );
  if( !obj->data().SharedGroup )
    return {};
  auto sg = getRow< Excel::ExportedSG >( obj->data().SharedGroup );
  if( !sg )
    throw std::runtime_error( "Missing ExportedSG " + std::to_string( obj->data().SharedGroup ) );
  auto path = sg->getString( sg->data().SGFilePath );
  if( path.empty() )
    throw std::runtime_error( "ExportedSG has no asset path" );
  return path;
}
