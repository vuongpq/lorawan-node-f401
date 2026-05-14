/*!
 * \file      board.c
 *
 * \brief     Board-level functions called by the LoRaMac stack.
 */
#include "stm32f4xx_hal.h"
#include "board.h"

void BoardInitMcu( void )
{
    /* HAL and peripherals are already initialised in main.c */
}

void BoardGetUniqueId( uint8_t *id )
{
    /* Keep byte order consistent with debug view in main.c so the DevEUI
     * observed in debugger matches the DevEUI used by the soft secure element. */
    uint32_t uid0 = HAL_GetUIDw0( );
    uint32_t uid1 = HAL_GetUIDw1( );
    uint32_t uid2 = HAL_GetUIDw2( );

    id[0] = ( uint8_t )( uid0 >> 24 );
    id[1] = ( uint8_t )( uid0 >> 16 );
    id[2] = ( uint8_t )( uid0 >> 8 );
    id[3] = ( uint8_t )( uid0 );
    id[4] = ( uint8_t )( uid1 >> 24 );
    id[5] = ( uint8_t )( uid1 >> 16 );
    id[6] = ( uint8_t )( uid2 >> 24 );
    id[7] = ( uint8_t )( uid2 >> 16 );
}

uint8_t BoardGetBatteryLevel( void )
{
    return 255; /* measurement not available */
}

uint32_t BoardGetRandomSeed( void )
{
    return ( HAL_GetUIDw0( ) ^ HAL_GetUIDw1( ) ^ HAL_GetUIDw2( ) );
}
