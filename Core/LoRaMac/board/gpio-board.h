/*!
 * \file      gpio-board.h
 *
 * \brief     GPIO MCU board-level driver (STM32F401, HAL).
 */
#ifndef __GPIO_BOARD_H__
#define __GPIO_BOARD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "gpio.h"

void GpioMcuInit( Gpio_t *obj, PinNames pin, PinModes mode, PinConfigs config,
                  PinTypes type, uint32_t value );

void GpioMcuSetContext( Gpio_t *obj, void *context );

void GpioMcuSetInterrupt( Gpio_t *obj, IrqModes irqMode, IrqPriorities irqPriority,
                          GpioIrqHandler *irqHandler );

void GpioMcuRemoveInterrupt( Gpio_t *obj );

void GpioMcuWrite( Gpio_t *obj, uint32_t value );

void GpioMcuToggle( Gpio_t *obj );

uint32_t GpioMcuRead( Gpio_t *obj );

#ifdef __cplusplus
}
#endif

#endif  /* __GPIO_BOARD_H__ */
