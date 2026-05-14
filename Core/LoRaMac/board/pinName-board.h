/*!
 * \file      pinName-board.h
 *
 * \brief     STM32F401 MCU GPIO pin name expansion for gpio.h PinNames enum.
 *            Encoding: port * 0x10 + pin_number
 *            PA_x = 0x00+x, PB_x = 0x10+x, PC_x = 0x20+x
 */
#ifndef __PIN_NAME_BOARD_H__
#define __PIN_NAME_BOARD_H__

#define MCU_PINS \
    PA_0 = 0x00, PA_1, PA_2, PA_3, PA_4, PA_5, PA_6, PA_7, \
    PA_8, PA_9, PA_10, PA_11, PA_12, PA_13, PA_14, PA_15,   \
    PB_0 = 0x10, PB_1, PB_2, PB_3, PB_4, PB_5, PB_6, PB_7, \
    PB_8, PB_9, PB_10, PB_11, PB_12, PB_13, PB_14, PB_15,   \
    PC_0 = 0x20, PC_1, PC_2, PC_3, PC_4, PC_5, PC_6, PC_7, \
    PC_8, PC_9, PC_10, PC_11, PC_12, PC_13, PC_14, PC_15

#endif  /* __PIN_NAME_BOARD_H__ */
