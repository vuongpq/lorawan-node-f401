/*!
 * \file      delay-board.h
 *
 * \brief     MCU delay board-level driver.
 */
#ifndef __DELAY_BOARD_H__
#define __DELAY_BOARD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/*!
 * \brief Blocking delay of \p ms milliseconds using the HAL tick.
 */
void DelayMsMcu( uint32_t ms );

#ifdef __cplusplus
}
#endif

#endif  /* __DELAY_BOARD_H__ */
