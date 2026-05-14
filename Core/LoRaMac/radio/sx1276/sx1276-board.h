/*!
 * \file      sx1276-board.h
 *
 * \brief     SX1276 board-level definitions for Blackpill + RA-01H.
 *
 *            RA-01H: PA_BOOST only, no TCXO, no antenna switch.
 *
 *            SPI:   hspi1  PA5=SCLK  PA6=MISO  PA7=MOSI
 *            NSS:   PA4   (software CS)
 *            RESET: PB0
 *            DIO0:  PA3   (EXTI3    - TxDone / RxDone)
 *            DIO1:  PA8   (EXTI9_5  - RxTimeout)
 */
#ifndef __SX1276_BOARD_H__
#define __SX1276_BOARD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include "sx1276.h"

/*!
 * Frequency boundary between LF and HF bands (Hz).
 * Used for RSSI offset selection and image calibration.
 */
#define RF_MID_BAND_THRESH      525000000UL

/*!
 * \brief Radio hardware register initialisation.
 *
 * Each entry is { modem, address, value }.
 */
#define RADIO_INIT_REGISTERS_VALUE                \
{                                                 \
    { MODEM_FSK,  REG_LNA,           0x23 },      \
    { MODEM_FSK,  REG_RXCONFIG,      0x1E },      \
    { MODEM_FSK,  REG_RSSICONFIG,    0xD2 },      \
    { MODEM_FSK,  REG_AFCFEI,        0x01 },      \
    { MODEM_FSK,  REG_PREAMBLEDETECT,0xAA },      \
    { MODEM_FSK,  REG_OSC,           0x07 },      \
    { MODEM_FSK,  REG_SYNCCONFIG,    0x12 },      \
    { MODEM_FSK,  REG_SYNCVALUE1,    0xC1 },      \
    { MODEM_FSK,  REG_SYNCVALUE2,    0x94 },      \
    { MODEM_FSK,  REG_SYNCVALUE3,    0xC1 },      \
    { MODEM_FSK,  REG_PACKETCONFIG1, 0xD8 },      \
    { MODEM_FSK,  REG_FIFOTHRESH,    0x8F },      \
    { MODEM_FSK,  REG_IMAGECAL,      0x02 },      \
    { MODEM_FSK,  REG_DIOMAPPING1,   0x00 },      \
    { MODEM_FSK,  REG_DIOMAPPING2,   0x30 },      \
    { MODEM_LORA, REG_LR_PAYLOADMAXLENGTH, 0x40 },\
}

/*!
 * Initialise SX1276 GPIO objects (Reset, DIO0, DIO1, SPI Nss) and SPI.
 * Must be called once before SX1276Init().
 */
void SX1276IoInit( void );

/*!
 * Toggle RESET low for 1 ms then release, waiting 6 ms for POR.
 */
void SX1276Reset( void );

/*!
 * Initialise the DIO pins and attach IRQ handlers.
 * \param [IN] irqHandlers  DioIrqHandler function pointer array (DIO0..DIO5).
 */
void SX1276IoIrqInit( DioIrqHandler **irqHandlers );

/*!
 * Set the RF output power.
 * \param [IN] power  dBm (-4..+20 for PA_BOOST on SX1276)
 */
void SX1276SetRfTxPower( int8_t power );

/*!
 * Set antenna switch low-power mode.  No switch on this board - NOP.
 */
void SX1276SetAntSwLowPower( bool status );

/*!
 * Select antenna switch state.  No switch on this board - NOP.
 */
void SX1276SetAntSw( uint8_t opMode );

/*!
 * Enable/disable TCXO.  No TCXO on RA-01H - NOP.
 */
void SX1276SetBoardTcxo( uint8_t state );

/*!
 * \return TCXO wake-up delay in ms (0 - no TCXO).
 */
uint32_t SX1276GetBoardTcxoWakeupTime( void );

/*!
 * \return true if frequency is within the SX1276 operating range.
 */
bool SX1276CheckRfFrequency( uint32_t frequency );

/*!
 * \return Current logic level of the DIO1 pin (0 or 1).
 */
uint8_t SX1276GetDio1PinState( void );

#ifdef __cplusplus
}
#endif

#endif  /* __SX1276_BOARD_H__ */
