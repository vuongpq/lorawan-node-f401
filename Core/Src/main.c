/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "spi.h"
#include "gpio.h"
#include "oled.h"
#include "Commissioning.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

volatile uint64_t gDevEuiValue = 0;
volatile uint64_t gJoinEuiValue = 0;
volatile uint64_t gAppKeyHigh = 0;
volatile uint64_t gAppKeyLow = 0;
char gDevEuiHexOnly[17] = "0000000000000000";
const char *gDevEuiHexText = gDevEuiHexOnly;
static uint32_t gLastMq7Update = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
__attribute__((weak)) void LoRaWAN_Init(void) { }
__attribute__((weak)) void LoRaWAN_Process(void) { }
__attribute__((weak)) bool LoRaWAN_IsJoined(void) { return false; }
__attribute__((weak)) void LoRaWAN_SetMQ7Raw(uint16_t adcRaw) { (void)adcRaw; }

static void DebugGetUniqueId( uint8_t *id )
{
  uint32_t id0 = HAL_GetUIDw0( );
  uint32_t id1 = HAL_GetUIDw1( );
  uint32_t id2 = HAL_GetUIDw2( );

  id[0] = ( uint8_t )( id0 >> 24 );
  id[1] = ( uint8_t )( id0 >> 16 );
  id[2] = ( uint8_t )( id0 >> 8 );
  id[3] = ( uint8_t )( id0 );
  id[4] = ( uint8_t )( id1 >> 24 );
  id[5] = ( uint8_t )( id1 >> 16 );
  id[6] = ( uint8_t )( id2 >> 24 );
  id[7] = ( uint8_t )( id2 >> 16 );
}

static uint64_t DebugPackU64( const uint8_t *data, uint8_t size )
{
  uint8_t i = 0;
  uint64_t value = 0;

  for( i = 0; i < size; i++ )
  {
    value = ( value << 8 ) | data[i];
  }

  return value;
}

static void DebugFormatHex( const uint8_t *data, uint8_t size, char *out )
{
  static const char hexTable[] = "0123456789ABCDEF";
  uint8_t i = 0;

  for( i = 0; i < size; i++ )
  {
    out[2U * i] = hexTable[( data[i] >> 4 ) & 0x0FU];
    out[(2U * i) + 1U] = hexTable[data[i] & 0x0FU];
  }
  out[2U * size] = '\0';
}

static void DebugUpdateIdentityValues(void)
{
  uint8_t devEui[8] = { 0 };

  DebugGetUniqueId( devEui );

  gDevEuiValue = DebugPackU64( devEui, 8U );
  DebugFormatHex( devEui, 8U, gDevEuiHexOnly );
  gDevEuiHexText = gDevEuiHexOnly;
  gJoinEuiValue = DebugPackU64( JoinEui, sizeof( JoinEui ) );
  gAppKeyHigh = DebugPackU64( AppKey, 8U );
  gAppKeyLow = DebugPackU64( &AppKey[8], 8U );
}

/* MQ-7 CO concentration estimation
 * Curve fit from datasheet:  ppm = A * (Rs/R0) ^ B
 * Circuit: Vc = 3.3 V, RL = 10 kOhm (typical module).
 * Calibration: heat sensor 60 s in clean air, then set MQ7_R0_KOHM = measured Rs.
 * Default R0 = 10 kOhm (Rs in air ≈ RL * (3.3-Vclean)/Vclean) */
#define MQ7_VREF      3.3f
#define MQ7_ADC_FULL  4095.0f
#define MQ7_RL_KOHM   10.0f
#define MQ7_R0_KOHM   10.0f   /* adjust after calibration in clean air */
#define MQ7_CURVE_A   99.042f
#define MQ7_CURVE_B   (-1.518f)

static uint32_t MQ7_AdcToPpm(uint16_t adcRaw)
{
  float vout;
  float rs;
  float ratio;
  float ppm;

  if (adcRaw == 0U)      { return 0U; }

  vout  = (float)adcRaw * MQ7_VREF / MQ7_ADC_FULL;
  if (vout >= MQ7_VREF)  { return 0U; }   /* open circuit */

  rs    = MQ7_RL_KOHM * (MQ7_VREF - vout) / vout;
  ratio = rs / MQ7_R0_KOHM;
  if (ratio <= 0.0f)     { return 9999U; }

  ppm   = MQ7_CURVE_A * powf(ratio, MQ7_CURVE_B);

  if (ppm < 0.0f)        { ppm = 0.0f; }
  if (ppm > 9999.0f)     { ppm = 9999.0f; }

  return (uint32_t)ppm;
}

static void MQ7_InitAdc(void)
{
  GPIO_InitTypeDef gpio = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_ADC1_CLK_ENABLE();

  gpio.Pin = GPIO_PIN_0;
  gpio.Mode = GPIO_MODE_ANALOG;
  gpio.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &gpio);

  ADC->CCR &= ~(ADC_CCR_ADCPRE_Msk);
  ADC->CCR |= ADC_CCR_ADCPRE_0; /* PCLK2/4 for ADC clock */

  ADC1->CR1 = 0;
  ADC1->CR2 = ADC_CR2_ADON;
  ADC1->SMPR2 &= ~(ADC_SMPR2_SMP0_Msk);
  ADC1->SMPR2 |= (3UL << ADC_SMPR2_SMP0_Pos); /* 56 cycles sample time */
  ADC1->SQR1 = 0;
  ADC1->SQR2 = 0;
  ADC1->SQR3 = 0; /* rank1 -> channel 0 (PA0) */
}

static uint16_t MQ7_ReadRaw(void)
{
  uint32_t startTick = HAL_GetTick();

  ADC1->SR = 0;
  ADC1->CR2 |= ADC_CR2_SWSTART;

  while ((ADC1->SR & ADC_SR_EOC) == 0U)
  {
    if ((HAL_GetTick() - startTick) > 10U)
    {
      return 0U;
    }
  }

  return (uint16_t)(ADC1->DR & 0x0FFFU);
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
  MQ7_InitAdc();
  OLED_Init();
  DebugUpdateIdentityValues();
  LoRaWAN_Init();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    uint32_t now = HAL_GetTick();
    if ((now - gLastMq7Update) >= 500U)
    {
      uint32_t rawAdc = 0;
      uint32_t milliVolt = 0;
      uint32_t ppm = 0;
      bool joined = false;
      char line1[22];
      char line2[22];
      char line3[22];
      char line4[22];

      gLastMq7Update = now;

      rawAdc    = MQ7_ReadRaw();
      milliVolt = (rawAdc * 3300U) / 4095U;
      ppm       = MQ7_AdcToPpm((uint16_t)rawAdc);
      joined    = LoRaWAN_IsJoined();

      LoRaWAN_SetMQ7Raw((uint16_t)rawAdc);  /* update payload for next uplink */

      (void)snprintf(line1, sizeof(line1), "MQ7 ADC:%4lu", (unsigned long)rawAdc);
      (void)snprintf(line2, sizeof(line2), "V:%lu.%03lu", (unsigned long)(milliVolt / 1000U), (unsigned long)(milliVolt % 1000U));
      (void)snprintf(line3, sizeof(line3), "CO:%4lu PPM", (unsigned long)ppm);
      (void)snprintf(line4, sizeof(line4), "LORAWAN %c",
                     joined ? OLED_CHAR_JOINED : OLED_CHAR_NOT_JOINED);

      OLED_Clear();
      OLED_DrawText(0U, 0U, line1);
      OLED_DrawText(0U, 2U, line2);
      OLED_DrawText(0U, 4U, line3);
      OLED_DrawText(0U, 6U, line4);
      OLED_Update();
    }

    DebugUpdateIdentityValues();
    LoRaWAN_Process();
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
