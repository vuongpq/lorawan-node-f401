/*!
 * \file      rtc-board.h
 *
 * \brief     RTC/Timer board layer for the LoRaMac timer scheduler.
 *
 *            Implemented on top of HAL_GetTick() (1 ms resolution, 32-bit
 *            wrap-around at ~49 days) plus a TIM2 hardware alarm that fires
 *            TimerIrqHandler() at the requested deadline.
 *
 *            Tick == milliseconds in this implementation
 *            (RtcMs2Tick / RtcTick2Ms are identity functions).
 */
#ifndef __RTC_BOARD_H__
#define __RTC_BOARD_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "timer.h"

/*!
 * Save current HAL tick as the timer context reference.
 * \return The saved tick value.
 */
uint32_t RtcSetTimerContext( void );

/*!
 * \return The HAL tick value saved by the last RtcSetTimerContext() call.
 */
uint32_t RtcGetTimerContext( void );

/*!
 * \return Milliseconds elapsed since the last RtcSetTimerContext() call.
 */
uint32_t RtcGetTimerElapsedTime( void );

/*!
 * \return Current HAL tick (milliseconds since boot).
 */
uint32_t RtcGetTimerValue( void );

/*!
 * Convert milliseconds to timer ticks (identity: 1 tick == 1 ms).
 */
uint32_t RtcMs2Tick( TimerTime_t milliseconds );

/*!
 * Convert timer ticks to milliseconds (identity: 1 tick == 1 ms).
 */
TimerTime_t RtcTick2Ms( uint32_t tick );

/*!
 * Stop the pending TIM2 alarm.
 */
void RtcStopAlarm( void );

/*!
 * Schedule TimerIrqHandler() to be called \p timeout ticks from the context.
 */
void RtcSetAlarm( uint32_t timeout );

/*!
 * \return Minimum allowed alarm timeout in ticks (1 ms).
 */
uint32_t RtcGetMinimumTimeout( void );

/*!
 * Temperature compensation stub (not required).
 */
TimerTime_t RtcTempCompensation( TimerTime_t period, float temperature );

/*!
 * Must be called from the TIM2 period-elapsed IRQ handler.
 */
void RtcAlarmIrqHandler( void );

/*!
 * Called from TimerProcess() – used for poll-mode RTC implementations.
 * This implementation is interrupt-driven so this is a NOP.
 */
void RtcProcess( void );

/*!
 * Read the "calendar" time. Returns seconds since boot; subSeconds is the
 * fractional part in milliseconds (0-999).
 */
uint32_t RtcGetCalendarTime( uint16_t *subSeconds );

/*!
 * Write a time offset to the simulated backup registers.
 */
void RtcBkupWrite( uint32_t seconds, uint32_t subSeconds );

/*!
 * Read the time offset from the simulated backup registers.
 */
void RtcBkupRead( uint32_t *seconds, uint32_t *subSeconds );

/*!
 * Initialise the TIM2-based alarm and board RTC substrate.
 */
void RtcBoardInit( void );

#ifdef __cplusplus
}
#endif

#endif  /* __RTC_BOARD_H__ */
