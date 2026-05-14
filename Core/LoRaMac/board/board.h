/*!
 * \file      board.h
 *
 * \brief     Board-level definitions used by the LoRaMac stack.
 *
 *            This header is the one that timer.c / soft-se-hal.c / etc.
 *            find via the `board` CMake target include path.  It pulls in
 *            the STM32 HAL and declares the functions implemented in
 *            Core/Src/board.c.
 */
#ifndef __BOARD_H__
#define __BOARD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_hal.h"
#include "utilities.h"

/*!
 * Generic board pin definitions used by the SX1276 board layer.
 * These match the physical wiring on the Blackpill + RA-01H.
 *   NSS   = PA4
 *   RESET = PB0
 *   DIO0  = PA3   (EXTI3)
 *   DIO1  = PA8   (EXTI9_5)
 */
#define RADIO_NSS             PA_4
#define RADIO_RESET           PB_0
#define RADIO_DIO0            PA_3
#define RADIO_DIO1            PA_8
#define RADIO_DIO2            NC
#define RADIO_DIO3            NC
#define RADIO_DIO4            NC
#define RADIO_DIO5            NC

/*!
 * Initialise board hardware (called once from main before the LoRaMac init).
 */
void BoardInitMcu( void );

/*!
 * Copy the 8-byte unique device ID derived from the STM32 UID registers.
 */
void BoardGetUniqueId( uint8_t *id );

/*!
 * \return Battery level [0-254] or 255 if measurement not available.
 */
uint8_t BoardGetBatteryLevel( void );

/*!
 * \return A 32-bit pseudo-random seed from the UID.
 */
uint32_t BoardGetRandomSeed( void );

#ifdef __cplusplus
}
#endif

#endif  /* __BOARD_H__ */
