#pragma once

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>

struct LayoutSection
{
  const char* data;
  size_t size;

  template< typename T >
  T read( size_t at ) const
  {
    if( at > size || size - at < sizeof( T ) )
      throw std::runtime_error( "Truncated layout record" );
    T value;
    std::memcpy( &value, data + at, sizeof( T ) );
    return value;
  }

  size_t relative( size_t base, int32_t offset ) const
  {
    const auto at = static_cast< int64_t >( base ) + offset;
    if( at < 0 || static_cast< uint64_t >( at ) > size )
      throw std::runtime_error( "Invalid layout offset" );
    return static_cast< size_t >( at );
  }

  size_t table( size_t base, int32_t offset, int32_t count, size_t stride ) const
  {
    if( !count )
      return base;
    const auto at = relative( base, offset );
    if( count < 0 || static_cast< size_t >( count ) > ( size - at ) / stride )
      throw std::runtime_error( "Invalid layout table" );
    return at;
  }

  std::string stringAt( size_t base, int32_t offset ) const
  {
    if( !offset )
      return {};
    const auto at = relative( base, offset );
    const char* start = data + at;
    const auto end = static_cast< const char* >( std::memchr( start, 0, size - at ) );
    if( !end )
      throw std::runtime_error( "Unterminated layout string" );
    return std::string( start, end );
  }

  size_t chunk( const char* magic, const char* type ) const
  {
    const auto firstChunk = read< uint32_t >( 12 );
    const bool extended = std::memcmp( magic, "LGB1", 4 ) == 0 && firstChunk == 1;
    auto at = extended ? 32u : 12u;
    if( std::memcmp( data, magic, 4 ) || at + 8 > size ||
        std::memcmp( data + at, type, 4 ) )
      throw std::runtime_error( "Unexpected layout format" );
    return at + ( extended ? 16 : 8 );
  }
};
