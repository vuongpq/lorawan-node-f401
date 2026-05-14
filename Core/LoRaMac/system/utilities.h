#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdbool.h>
#include <stdint.h>

#include "stm32f4xx.h"

typedef enum LmnStatus_e
{
	LMN_STATUS_ERROR = 0,
	LMN_STATUS_OK    = !LMN_STATUS_ERROR
} LmnStatus_t;

#ifndef MIN
#define MIN( a, b )                                ( ( ( a ) < ( b ) ) ? ( a ) : ( b ) )
#endif

#ifndef MAX
#define MAX( a, b )                                ( ( ( a ) > ( b ) ) ? ( a ) : ( b ) )
#endif

#define CRITICAL_SECTION_BEGIN() uint32_t primask_bit = __get_PRIMASK(); __disable_irq()
#define CRITICAL_SECTION_BEGIN_REPEAT() uint32_t primask_bit = __get_PRIMASK(); __disable_irq()
#define CRITICAL_SECTION_END() if( primask_bit == 0U ) { __enable_irq(); }

void memcpy1( uint8_t *dst, const uint8_t *src, uint16_t size );
void memcpyr( uint8_t *dst, const uint8_t *src, uint16_t size );
void memset1( uint8_t *dst, uint8_t value, uint16_t size );

int32_t rand1( void );
void srand1( uint32_t seed );
int32_t randr( int32_t min, int32_t max );

char Nibble2HexChar( uint8_t a );
uint32_t Crc32( const uint8_t *buffer, uint16_t length );

void BoardGetUniqueId( uint8_t *id );

#endif /* UTILITIES_H */
