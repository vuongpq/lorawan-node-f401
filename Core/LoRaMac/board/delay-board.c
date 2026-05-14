/*!
 * \file      delay-board.c
 *
 * \brief     MCU delay board-level driver for STM32F401 using HAL_Delay().
 */
#include "stm32f4xx_hal.h"
#include "delay-board.h"

void DelayMsMcu( uint32_t ms )
{
    HAL_Delay( ms );
}
