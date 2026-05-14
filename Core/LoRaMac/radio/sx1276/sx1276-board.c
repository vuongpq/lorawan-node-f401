/*!
 * \file      sx1276-board.c
 *
 * \brief     SX1276 board-level driver for Blackpill STM32F401 + RA-01H.
 *
 *            SPI1:   SCLK=PA5, MISO=PA6, MOSI=PA7
 *            NSS:    PA4   (manual CS)
 *            RESET:  PB0
 *            DIO0:   PA3   (EXTI3   - TxDone / RxDone)
 *            DIO1:   PA8   (EXTI9_5 - RxTimeout)
 */
#include "stm32f4xx_hal.h"
#include "sx1276-board.h"
#include "gpio-board.h"
#include "gpio.h"
#include "spi.h"
#include "delay.h"

extern SPI_HandleTypeDef hspi1;

/* SX1276 global struct defined in sx1276.c */
extern SX1276_t SX1276;

/* DioIrq table populated by SX1276IoIrqInit() */
static DioIrqHandler **BoardDioIrqHandlers;

/* Debug: watch in debugger to verify DIO callbacks are dispatched */
volatile uint32_t gDbgDio0IrqCount = 0;
volatile uint32_t gDbgDio1IrqCount = 0;

/* -----------------------------------------------------------------------
 * SX1276IoInit - wire up GPIO and SPI objects in the global SX1276 struct
 * -------------------------------------------------------------------- */

void SX1276IoInit( void )
{
    /* RESET - PB0, output, push-pull, no pull, initially HIGH (not in reset) */
    GpioInit( &SX1276.Reset, PB_0, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 1 );

    /* DIO0 - PA3, input, no pull (EXTI configured later by SX1276IoIrqInit) */
    GpioInit( &SX1276.DIO0, PA_3, PIN_INPUT, PIN_PUSH_PULL, PIN_PULL_DOWN, 0 );
    /* DIO1 - PA8, input, no pull */
    GpioInit( &SX1276.DIO1, PA_8, PIN_INPUT, PIN_PUSH_PULL, PIN_PULL_DOWN, 0 );

    /* DIO2..5 set to NC so the sx1276.c null-pointer guard works */
    SX1276.DIO2.pin = NC;
    SX1276.DIO3.pin = NC;
    SX1276.DIO4.pin = NC;
    SX1276.DIO5.pin = NC;

    /* SPI - PA7=MOSI, PA6=MISO, PA5=SCLK.  NSS is handled manually via PA4. */
    SpiInit( &SX1276.Spi, SPI_1, PA_7, PA_6, PA_5, NC );

    /* Set NSS object manually so SX1276WriteBuffer / SX1276ReadBuffer can
     * drive it via GpioWrite().  The actual GPIO was already configured by
     * MX_GPIO_Init() in gpio.c.                                             */
    SX1276.Spi.Nss.pin      = PA_4;
    SX1276.Spi.Nss.port     = GPIOA;
    SX1276.Spi.Nss.pinIndex = GPIO_PIN_4;
}

/* -----------------------------------------------------------------------
 * SX1276Reset - toggle RESET low then release and wait for POR
 * -------------------------------------------------------------------- */

void SX1276Reset( void )
{
    /* Drive RESET low for 1 ms */
    GpioInit( &SX1276.Reset, PB_0, PIN_OUTPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    DelayMs( 1 );
    /* Release (set high) */
    GpioInit( &SX1276.Reset, PB_0, PIN_INPUT, PIN_PUSH_PULL, PIN_NO_PULL, 0 );
    DelayMs( 6 );
}

/* -----------------------------------------------------------------------
 * DIO IRQ callbacks (called from gpio-board.c dispatcher)
 * -------------------------------------------------------------------- */

static void Dio0IrqHandler( void *context )
{
    (void)context;
    gDbgDio0IrqCount++;
    if( BoardDioIrqHandlers && BoardDioIrqHandlers[0] )
    {
        BoardDioIrqHandlers[0]( NULL );
    }
}

static void Dio1IrqHandler( void *context )
{
    (void)context;
    gDbgDio1IrqCount++;
    if( BoardDioIrqHandlers && BoardDioIrqHandlers[1] )
    {
        BoardDioIrqHandlers[1]( NULL );
    }
}

/* -----------------------------------------------------------------------
 * SX1276IoIrqInit - wire DIO0 and DIO1 to their handlers
 * -------------------------------------------------------------------- */

void SX1276IoIrqInit( DioIrqHandler **irqHandlers )
{
    BoardDioIrqHandlers = irqHandlers;

    /* DIO0 - PA3, rising edge (TxDone / RxDone) */
    GpioSetInterrupt( &SX1276.DIO0, IRQ_RISING_EDGE,
                      IRQ_HIGH_PRIORITY, Dio0IrqHandler );
    /* DIO1 - PA8, both edges.
     * Semtech handler checks pin level to filter valid edge for each mode. */
    GpioSetInterrupt( &SX1276.DIO1, IRQ_RISING_FALLING_EDGE,
                      IRQ_HIGH_PRIORITY, Dio1IrqHandler );
}

/* -----------------------------------------------------------------------
 * PA / power control
 * RA-01H uses the PA_BOOST pin; max +20 dBm (PACONFIG 0x8F + PADAC 0x87).
 * -------------------------------------------------------------------- */

void SX1276SetRfTxPower( int8_t power )
{
    uint8_t paConfig = 0;
    uint8_t paDac    = 0;

    /* PA_BOOST path only (no RFO pin on RA-01H) */
    paConfig = 0x80; /* PaSelect = PA_BOOST */

    if( power > 17 )
    {
        /* High-power mode: 20 dBm max */
        if( power > 20 ) { power = 20; }
        paDac   = 0x87; /* PA_DAC_20_DBM */
        paConfig |= ( uint8_t )( power - 5 ) & 0x0F;
    }
    else
    {
        /* Normal mode: 2..17 dBm */
        if( power < 2 ) { power = 2; }
        paDac = 0x84; /* PA_DAC_DEFAULT */
        paConfig |= ( uint8_t )( power - 2 ) & 0x0F;
    }

    SX1276Write( REG_LR_PACONFIG, paConfig );
    SX1276Write( REG_LR_PADAC,    paDac    );
}

/* -----------------------------------------------------------------------
 * Antenna switch / TCXO stubs (no hardware on RA-01H)
 * -------------------------------------------------------------------- */

void SX1276SetAntSwLowPower( bool status )
{
    (void)status;
}

void SX1276SetAntSw( uint8_t opMode )
{
    (void)opMode;
}

void SX1276SetBoardTcxo( uint8_t state )
{
    (void)state;
}

uint32_t SX1276GetBoardTcxoWakeupTime( void )
{
    return 0;
}

/* -----------------------------------------------------------------------
 * Frequency check
 * -------------------------------------------------------------------- */

bool SX1276CheckRfFrequency( uint32_t frequency )
{
    /* SX1276 supports 137-1020 MHz; RA-01H is rated 410-525 / 862-1020 MHz */
    return ( frequency >= 410000000UL && frequency <= 1020000000UL );
}

uint8_t SX1276GetDio1PinState( void )
{
    return ( uint8_t )HAL_GPIO_ReadPin( GPIOA, GPIO_PIN_8 );
}

/* -----------------------------------------------------------------------
 * Radio driver object – referenced via 'extern const struct Radio_s Radio'
 * in radio.h; consumed by LoRaMac.c
 * -------------------------------------------------------------------- */

const struct Radio_s Radio =
{
    SX1276Init,
    SX1276GetStatus,
    SX1276SetModem,
    SX1276SetChannel,
    SX1276IsChannelFree,
    SX1276Random,
    SX1276SetRxConfig,
    SX1276SetTxConfig,
    SX1276CheckRfFrequency,
    SX1276GetTimeOnAir,
    SX1276Send,
    SX1276SetSleep,
    SX1276SetStby,
    SX1276SetRx,
    SX1276StartCad,
    SX1276SetTxContinuousWave,
    SX1276ReadRssi,
    SX1276Write,
    SX1276Read,
    SX1276WriteBuffer,
    SX1276ReadBuffer,
    SX1276SetMaxPayloadLength,
    SX1276SetPublicNetwork,
    SX1276GetWakeupTime,
    NULL,   /* IrqProcess  - interrupt-driven, not polled */
    NULL,   /* RxBoosted   - SX126x only */
    NULL,   /* SetRxDutyCycle - SX126x only */
};
