#include "board.h"

#include <stdint.h>

#include "stm32f4xx_hal.h"

void Board_Init(void)
{
    /* Board-level initialization hook */
}

void BoardGetUniqueId( uint8_t *id )
{
    uint32_t id0 = HAL_GetUIDw0( );
    uint32_t id1 = HAL_GetUIDw1( );
    uint32_t id2 = HAL_GetUIDw2( );

    id[0] = ( uint8_t )( id0 >> 24 );
    id[1] = ( uint8_t )( id0 >> 16 );
    id[2] = ( uint8_t )( id0 >> 8 );
    id[3] = ( uint8_t )( id0 );
    id[4] = ( uint8_t )( id1 >> 24 );
    id[5] = ( uint8_t )( id1 >> 16 );
    id[6] = ( uint8_t )( id2 >> 24 );
    id[7] = ( uint8_t )( id2 >> 16 );
}
