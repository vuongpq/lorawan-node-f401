/*!
 * \file      pinName-ioe.h
 *
 * \brief     IO expander pin names stub (no IO expander on this board).
 *
 *            gpio.h declares the PinNames enum as:
 *              { MCU_PINS, IOE_PINS, NC = 0xFFFFFFFF }
 *            IOE_PINS must expand to at least one identifier so that the
 *            trailing comma on MCU_PINS does not produce a double-comma.
 */
#ifndef __PIN_NAME_IOE_H__
#define __PIN_NAME_IOE_H__

/* Dummy placeholder - absorbs the comma that follows in the enum */
#define IOE_PINS  IOE_PIN_NONE = 0xFFFE

#endif  /* __PIN_NAME_IOE_H__ */
