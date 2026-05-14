/*!
 * \file      rtc-board.c
 *
 * \brief     RTC/Timer board layer for STM32F401.
 *
 *            Timestamps  : HAL_GetTick()  (1 ms SysTick, always available)
 *            Alarm timer : TIM2 in one-pulse mode at 1 MHz (direct registers,
 *                          no HAL_TIM dependency so we don'\''t need
 *                          stm32f4xx_hal_tim.c).
 *
 *            APB1 clock  = 42 MHz, APB1 timer clock = 84 MHz.
 *            Prescaler   = 83 ? counter @ 1 MHz ? 1 �s per tick.
 *            Maximum one-shot delay � 4295 s (32-bit ARR).
 */
#include "stm32f4xx_hal.h"   /* HAL_GetTick, __HAL_RCC_TIM2_CLK_ENABLE */
#include "rtc-board.h"
#include "timer.h"

/* Saved context timestamp (ms) */
static volatile uint32_t RtcTimerContext = 0;
static volatile uint32_t RtcAlarmDeadline = 0;
static volatile bool     RtcAlarmArmed = false;

static uint32_t Tim2GetClockHz( void )
{
    RCC_ClkInitTypeDef clkInit;
    uint32_t flashLatency;
    uint32_t timClkHz = HAL_RCC_GetPCLK1Freq( );

    HAL_RCC_GetClockConfig( &clkInit, &flashLatency );
    if( clkInit.APB1CLKDivider != RCC_HCLK_DIV1 )
    {
        timClkHz *= 2U;
    }

    return timClkHz;
}

/* -------------------------------------------------------------------------
 * Internal helpers � direct TIM2 register access
 * ---------------------------------------------------------------------- */

static void Tim2Init( void )
{
    /* Enable TIM2 peripheral clock */
    __HAL_RCC_TIM2_CLK_ENABLE( );

    /* Stop counter, reset */
    TIM2->CR1  = 0;
    TIM2->DIER = 0;
    TIM2->SR   = 0;

    /* Set counter clock to 1 MHz from current APB1 timer clock */
    uint32_t timClkHz = Tim2GetClockHz( );
    uint32_t psc = ( timClkHz / 1000000U );
    TIM2->PSC = ( psc > 0U ) ? ( psc - 1U ) : 0U;

    /* Enable TIM2 update-interrupt in NVIC */
    HAL_NVIC_SetPriority( TIM2_IRQn, 2, 0 );
    HAL_NVIC_EnableIRQ( TIM2_IRQn );
}

static void Tim2StartOneShot( uint32_t usec ) __attribute__((unused));
static void Tim2StartOneShot( uint32_t usec )
{
    /* Stop and reset */
    TIM2->CR1  = 0;
    TIM2->DIER = 0;
    TIM2->SR   = 0;
    TIM2->CNT  = 0;

    /* Re-apply prescaler (must be reloaded after stop) */
    uint32_t timClkHz = Tim2GetClockHz( );
    uint32_t psc = ( timClkHz / 1000000U );
    TIM2->PSC  = ( psc > 0U ) ? ( psc - 1U ) : 0U;

    /* Period: usec - 1 counts (32-bit, so max � 4295 s) */
    TIM2->ARR  = ( usec > 0U ) ? ( usec - 1U ) : 0U;

    /* One-pulse mode, update interrupt enable, start */
    TIM2->DIER = TIM_DIER_UIE;
    TIM2->CR1  = TIM_CR1_OPM | TIM_CR1_CEN;

    /* Force PSC/ARR shadow-register update */
    TIM2->EGR  = TIM_EGR_UG;
    TIM2->SR   = 0;  /* clear the UIF set by UG */
    TIM2->CR1  = TIM_CR1_OPM | TIM_CR1_CEN;
}

static void Tim2Stop( void )
{
    TIM2->CR1  &= ~TIM_CR1_CEN;
    TIM2->DIER  =  0;
    TIM2->SR    =  0;
}

/* -------------------------------------------------------------------------
 * Context helpers
 * ---------------------------------------------------------------------- */

uint32_t RtcSetTimerContext( void )
{
    RtcTimerContext = HAL_GetTick( );
    return RtcTimerContext;
}

uint32_t RtcGetTimerContext( void )
{
    return RtcTimerContext;
}

uint32_t RtcGetTimerElapsedTime( void )
{
    return ( uint32_t )( HAL_GetTick( ) - RtcTimerContext );
}

uint32_t RtcGetTimerValue( void )
{
    return HAL_GetTick( );
}

/* -------------------------------------------------------------------------
 * Tick / ms conversion (1 tick == 1 ms)
 * ---------------------------------------------------------------------- */

uint32_t RtcMs2Tick( TimerTime_t milliseconds )
{
    return ( uint32_t )milliseconds;
}

TimerTime_t RtcTick2Ms( uint32_t tick )
{
    return ( TimerTime_t )tick;
}

uint32_t RtcGetMinimumTimeout( void )
{
    return 1;
}

TimerTime_t RtcTempCompensation( TimerTime_t period, float temperature )
{
    (void)temperature;
    return period;
}

/* -------------------------------------------------------------------------
 * TIM2 alarm
 * ---------------------------------------------------------------------- */

/* Called once from lora_app.c before LoRaMacInitialization */
void RtcBoardInit( void )
{
    Tim2Init( );
}

void RtcStopAlarm( void )
{
    Tim2Stop( );
    RtcAlarmArmed = false;
}

void RtcSetAlarm( uint32_t timeout )
{
    uint32_t now     = HAL_GetTick( );
    uint32_t elapsed = now - RtcTimerContext;
    uint32_t remain;

    if( timeout > elapsed )
    {
        remain = timeout - elapsed;
    }
    else
    {
        remain = 1;
    }
    if( remain == 0 )
    {
        remain = 1;
    }

    /* Polling fallback deadline in ms tick domain */
    RtcAlarmDeadline = now + remain;
    RtcAlarmArmed = true;

    /* Keep TIM2 stopped and let RtcProcess service alarms from HAL_GetTick.
     * This avoids sporadic late one-shot expirations observed on some boards. */
    Tim2Stop( );
}

/* -------------------------------------------------------------------------
 * IRQ entry point � called directly from TIM2_IRQHandler in stm32f4xx_it.c
 * ---------------------------------------------------------------------- */

void RtcAlarmIrqHandler( void )
{
    Tim2Stop( );
    RtcAlarmArmed = false;
    TimerIrqHandler( );
}

/* -------------------------------------------------------------------------
 * RtcProcess – NOP for interrupt-driven implementations
 * ---------------------------------------------------------------------- */

void RtcProcess( void )
{
    /* Polling fallback when TIM2 IRQ is not delivered. */
    if( RtcAlarmArmed == true )
    {
        int32_t diff = ( int32_t )( HAL_GetTick( ) - RtcAlarmDeadline );
        if( diff >= 0 )
        {
            RtcAlarmArmed = false;
            Tim2Stop( );
            TimerIrqHandler( );
        }
    }
}

/* -------------------------------------------------------------------------
 * Calendar / backup-register emulation for systime.c
 * ---------------------------------------------------------------------- */

static uint32_t RtcBkupSeconds    = 0;
static uint32_t RtcBkupSubSeconds = 0;

uint32_t RtcGetCalendarTime( uint16_t *subSeconds )
{
    uint32_t ticks = HAL_GetTick( );
    if( subSeconds != NULL )
    {
        *subSeconds = ( uint16_t )( ticks % 1000U );
    }
    return ticks / 1000U;
}

void RtcBkupWrite( uint32_t seconds, uint32_t subSeconds )
{
    RtcBkupSeconds    = seconds;
    RtcBkupSubSeconds = subSeconds;
}

void RtcBkupRead( uint32_t *seconds, uint32_t *subSeconds )
{
    if( seconds != NULL )
    {
        *seconds = RtcBkupSeconds;
    }
    if( subSeconds != NULL )
    {
        *subSeconds = RtcBkupSubSeconds;
    }
}
