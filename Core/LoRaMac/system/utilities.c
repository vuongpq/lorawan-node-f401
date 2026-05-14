#include "utilities.h"

#include <stdlib.h>
#include <string.h>

void memcpy1( uint8_t *dst, const uint8_t *src, uint16_t size )
{
    ( void )memcpy( dst, src, size );
}

void memcpyr( uint8_t *dst, const uint8_t *src, uint16_t size )
{
    for( uint16_t i = 0; i < size; i++ )
    {
        dst[i] = src[size - 1U - i];
    }
}

void memset1( uint8_t *dst, uint8_t value, uint16_t size )
{
    ( void )memset( dst, value, size );
}

int32_t rand1( void )
{
    return ( int32_t )rand( );
}

void srand1( uint32_t seed )
{
    srand( ( unsigned int )seed );
}

int32_t randr( int32_t min, int32_t max )
{
    if( max <= min )
    {
        return min;
    }

    return ( rand1( ) % ( max - min + 1 ) ) + min;
}

char Nibble2HexChar( uint8_t a )
{
    if( a < 10U )
    {
        return ( char )( '0' + a );
    }
    return ( char )( 'A' + ( a - 10U ) );
}

uint32_t Crc32( const uint8_t *buffer, uint16_t length )
{
    uint32_t crc = 0xFFFFFFFFU;

    for( uint16_t i = 0; i < length; i++ )
    {
        crc ^= buffer[i];
        for( uint8_t bit = 0; bit < 8U; bit++ )
        {
            if( ( crc & 1U ) != 0U )
            {
                crc = ( crc >> 1U ) ^ 0xEDB88320U;
            }
            else
            {
                crc >>= 1U;
            }
        }
    }

    return ~crc;
}
