/*!
 * \file      lora_app.c
 *
 * \brief     LoRaWAN OTAA application using LoRaMac 4.x API.
 *
 *            Implements LoRaWAN_Init() and LoRaWAN_Process() which override
 *            the weak stubs in main.c.
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#include "stm32f4xx_hal.h"
#include "main.h"
#include "Commissioning.h"

/* LoRaMac stack */
#include "LoRaMac.h"
#include "board.h"
#include "region/Region.h"
#include "timer.h"

/* Forward declarations only - do NOT include gpio-board.h or sx1276-board.h here
 * because those headers pull in LoRaMac/system/gpio.h which conflicts with the
 * CubeMX Core/Inc/gpio.h (same filename, different content). */
extern void GpioMcuIrqHandler( uint16_t gpioPin );
extern void SX1276IoInit( void );
extern void RtcBoardInit( void );

/* -----------------------------------------------------------------------
 * Application state machine
 * ---------------------------------------------------------------------- */
typedef enum
{
    APP_STATE_IDLE = 0,
    APP_STATE_JOIN,
    APP_STATE_JOINED,
    APP_STATE_SEND,
    APP_STATE_WAIT,
} AppState_t;

static AppState_t AppState     = APP_STATE_IDLE;
static bool       JoinAccepted = false;
static uint32_t   NextTxAt     = 0;
static uint8_t    UplinkBuf[64];
static uint32_t   UplinkCnt    = 0;
static uint16_t   gMQ7Raw      = 0;  /* latest ADC value from main */
static LoRaMacPrimitives_t gLoRaMacPrimitives;
static LoRaMacCallback_t   gLoRaMacCallbacks;
static uint8_t             gAppKeyBuf[16];

#define LORAWAN_APP_PORT       2
#define UPLINK_INTERVAL_MS     20000UL
#define JOIN_RETRY_MS          5000UL
#define JOIN_CONFIRM_WAIT_MS   15000UL
#define JOIN_ACCEPT_DELAY1_MS  5000UL
#define JOIN_ACCEPT_DELAY2_MS  6000UL
#define RX_TIMING_ERROR_MS     40UL
#define RX_MIN_SYMBOLS         12U
#define RX2_FREQUENCY_AS923    923200000UL
#define RX2_DR_AS923           DR_2

/* -----------------------------------------------------------------------
 * HAL GPIO EXTI callback � dispatch to gpio-board handler
 * ---------------------------------------------------------------------- */
void HAL_GPIO_EXTI_Callback( uint16_t GPIO_Pin )
{
    GpioMcuIrqHandler( GPIO_Pin );
}



/* -----------------------------------------------------------------------
 * LoRaMac primitive callbacks
 * ---------------------------------------------------------------------- */
static void OnMcpsConfirm( McpsConfirm_t *c )
{
    (void)c;
    NextTxAt = HAL_GetTick( ) + UPLINK_INTERVAL_MS;
    AppState = APP_STATE_WAIT;
}

static void OnMcpsIndication( McpsIndication_t *ind )
{
    (void)ind;
}

static void OnMlmeConfirm( MlmeConfirm_t *c )
{
    if( c->MlmeRequest == MLME_JOIN )
    {
        if( c->Status == LORAMAC_EVENT_INFO_STATUS_OK )
        {
            /* LED PC13 ON (active low) = join success */
            HAL_GPIO_WritePin( GPIOC, GPIO_PIN_13, GPIO_PIN_RESET );
            JoinAccepted = true;
            AppState     = APP_STATE_SEND;
            NextTxAt     = HAL_GetTick( ) + 1000;
        }
        else
        {
            /* LED PC13 OFF = join failed, will retry */
            HAL_GPIO_WritePin( GPIOC, GPIO_PIN_13, GPIO_PIN_SET );
            /* Retry after 30 s */
            AppState = APP_STATE_JOIN;
            NextTxAt = HAL_GetTick( ) + 30000;
        }
    }
}

static void OnMlmeIndication( MlmeIndication_t *ind )
{
    if( ind->MlmeIndication == MLME_REVERT_JOIN )
    {
        JoinAccepted = false;
        AppState     = APP_STATE_JOIN;
        NextTxAt     = HAL_GetTick( );
    }
}



/* -----------------------------------------------------------------------
 * Issue a JOIN request
 * ---------------------------------------------------------------------- */
static void SendJoinRequest( void )
{
    MlmeReq_t req;
    LoRaMacStatus_t st;
    req.Type                       = MLME_JOIN;
    req.Req.Join.NetworkActivation = ACTIVATION_TYPE_OTAA;
    req.Req.Join.Datarate          = DR_2;
    st = LoRaMacMlmeRequest( &req );

    if( st == LORAMAC_STATUS_OK )
    {
        /* Request accepted by MAC. Wait for MlmeConfirm instead of re-sending
         * every loop iteration. */
        AppState = APP_STATE_WAIT;
        NextTxAt = HAL_GetTick( ) + JOIN_CONFIRM_WAIT_MS;
    }
    else
    {
        /* MAC busy/restricted: back off and retry later. */
        AppState = APP_STATE_WAIT;
        if( req.ReqReturn.DutyCycleWaitTime > 0 )
        {
            NextTxAt = HAL_GetTick( ) + req.ReqReturn.DutyCycleWaitTime;
        }
        else
        {
            NextTxAt = HAL_GetTick( ) + JOIN_RETRY_MS;
        }
    }
}

/* -----------------------------------------------------------------------
 * Send an unconfirmed uplink
 * Payload (2 bytes, big-endian):
 *   Byte 0 : MSB of MQ-7 ADC raw (0-4095)
 *   Byte 1 : LSB of MQ-7 ADC raw
 * ---------------------------------------------------------------------- */
static void SendUplink( void )
{
    McpsReq_t        req;
    LoRaMacTxInfo_t  txInfo;
    uint16_t         raw = gMQ7Raw;

    UplinkBuf[0] = (uint8_t)((raw >> 8U) & 0xFFU);  /* MSB */
    UplinkBuf[1] = (uint8_t)( raw        & 0xFFU);  /* LSB */
    uint8_t payloadLen = 2U;

    if( LoRaMacQueryTxPossible( payloadLen, &txInfo ) != LORAMAC_STATUS_OK )
    {
        /* MAC commands pending — flush with empty frame */
        req.Type                        = MCPS_UNCONFIRMED;
        req.Req.Unconfirmed.fBuffer     = NULL;
        req.Req.Unconfirmed.fBufferSize = 0;
        req.Req.Unconfirmed.Datarate    = DR_2;
    }
    else
    {
        req.Type                        = MCPS_UNCONFIRMED;
        req.Req.Unconfirmed.fPort       = LORAWAN_APP_PORT;
        req.Req.Unconfirmed.fBuffer     = UplinkBuf;
        req.Req.Unconfirmed.fBufferSize = payloadLen;
        req.Req.Unconfirmed.Datarate    = DR_2;
    }

    LoRaMacStatus_t st = LoRaMacMcpsRequest( &req );
    if( st == LORAMAC_STATUS_OK || st == LORAMAC_STATUS_DUTYCYCLE_RESTRICTED )
    {
        UplinkCnt++;
    }
    AppState = APP_STATE_WAIT;
}

/* -----------------------------------------------------------------------
 * LoRaWAN_Init � called once from main()
 * ---------------------------------------------------------------------- */
void LoRaWAN_Init( void )
{
    memset( &gLoRaMacPrimitives, 0, sizeof( gLoRaMacPrimitives ) );
    memset( &gLoRaMacCallbacks, 0, sizeof( gLoRaMacCallbacks ) );
    MibRequestConfirm_t mib;

    /* PC13 LED (Blackpill onboard LED, active low) - OFF by default */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef ledGpio = { GPIO_PIN_13, GPIO_MODE_OUTPUT_PP,
                                 GPIO_NOPULL, GPIO_SPEED_FREQ_LOW, 0 };
    HAL_GPIO_Init( GPIOC, &ledGpio );
    HAL_GPIO_WritePin( GPIOC, GPIO_PIN_13, GPIO_PIN_SET );  /* OFF */

    /* Initialise TIM2 alarm timer (direct register access, no HAL TIM needed) */
    extern void RtcBoardInit( void );
    RtcBoardInit( );

    /* Bring SX1276 SPI/GPIO objects up before Radio.Init is called */
    SX1276IoInit( );

    /* Wire callbacks */
    gLoRaMacPrimitives.MacMcpsConfirm    = OnMcpsConfirm;
    gLoRaMacPrimitives.MacMcpsIndication = OnMcpsIndication;
    gLoRaMacPrimitives.MacMlmeConfirm    = OnMlmeConfirm;
    gLoRaMacPrimitives.MacMlmeIndication = OnMlmeIndication;

    LoRaMacInitialization( &gLoRaMacPrimitives,
                           &gLoRaMacCallbacks,
                           LORAMAC_REGION_AS923 );

    /* Public network (ChirpStack default) */
    mib.Type = MIB_PUBLIC_NETWORK;
    mib.Param.EnablePublicNetwork = true;
    LoRaMacMibSetRequestConfirm( &mib );

    /* ADR on */
    mib.Type = MIB_ADR;
    mib.Param.AdrEnable = true;
    LoRaMacMibSetRequestConfirm( &mib );

    /* Make join receive timing explicit and a bit more tolerant. */
    mib.Type = MIB_JOIN_ACCEPT_DELAY_1;
    mib.Param.JoinAcceptDelay1 = JOIN_ACCEPT_DELAY1_MS;
    LoRaMacMibSetRequestConfirm( &mib );

    mib.Type = MIB_JOIN_ACCEPT_DELAY_2;
    mib.Param.JoinAcceptDelay2 = JOIN_ACCEPT_DELAY2_MS;
    LoRaMacMibSetRequestConfirm( &mib );

    mib.Type = MIB_SYSTEM_MAX_RX_ERROR;
    mib.Param.SystemMaxRxError = RX_TIMING_ERROR_MS;
    LoRaMacMibSetRequestConfirm( &mib );

    mib.Type = MIB_MIN_RX_SYMBOLS;
    mib.Param.MinRxSymbols = RX_MIN_SYMBOLS;
    LoRaMacMibSetRequestConfirm( &mib );

    mib.Type = MIB_RX2_CHANNEL;
    mib.Param.Rx2Channel.Frequency = RX2_FREQUENCY_AS923;
    mib.Param.Rx2Channel.Datarate = RX2_DR_AS923;
    LoRaMacMibSetRequestConfirm( &mib );

    /* AppKey (= NwkKey for LoRaWAN 1.0.x compatibility) */
    memcpy( gAppKeyBuf, AppKey, sizeof( gAppKeyBuf ) );

    mib.Type = MIB_APP_KEY;
    mib.Param.AppKey = gAppKeyBuf;
    LoRaMacMibSetRequestConfirm( &mib );

    mib.Type = MIB_NWK_KEY;
    mib.Param.NwkKey = gAppKeyBuf;
    LoRaMacMibSetRequestConfirm( &mib );

    LoRaMacStart( );

    AppState = APP_STATE_JOIN;
    NextTxAt = HAL_GetTick( );
}

/* -----------------------------------------------------------------------
 * LoRaWAN_Process � called from main() while(1) loop
 * ---------------------------------------------------------------------- */
void LoRaWAN_Process( void )
{
    TimerProcess( );
    LoRaMacProcess( );

    uint32_t now = HAL_GetTick( );

    switch( AppState )
    {
        case APP_STATE_JOIN:
            if( now >= NextTxAt )
            {
                SendJoinRequest( );
            }
            break;

        case APP_STATE_SEND:
        case APP_STATE_JOINED:
            if( now >= NextTxAt )
            {
                SendUplink( );
            }
            break;

        case APP_STATE_WAIT:
            if( now >= NextTxAt )
            {
                AppState = JoinAccepted ? APP_STATE_SEND : APP_STATE_JOIN;
            }
            break;

        default:
            break;
    }
}

bool LoRaWAN_IsJoined( void )
{
    return JoinAccepted;
}

void LoRaWAN_SetMQ7Raw( uint16_t adcRaw )
{
    gMQ7Raw = adcRaw;
}
