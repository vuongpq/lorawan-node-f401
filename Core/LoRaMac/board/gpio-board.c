/*!
 * \file      gpio-board.c
 *
 * \brief     GPIO MCU board-level driver for STM32F401 using STM32 HAL.
 *
 *            PinNames encoding: port = pin >> 4, bit = pin & 0x0F
 *            PA_x = 0x00+x, PB_x = 0x10+x, PC_x = 0x20+x
 */
#include "stm32f4xx_hal.h"
#include "gpio-board.h"

/* Map port index to GPIO_TypeDef pointer */
static GPIO_TypeDef *PinToPort( PinNames pin )
{
    switch( ( pin >> 4 ) & 0x0F )
    {
        case 0:  return GPIOA;
        case 1:  return GPIOB;
        case 2:  return GPIOC;
        default: return NULL;
    }
}

static uint16_t PinToMask( PinNames pin )
{
    return ( uint16_t )( 1U << ( pin & 0x0F ) );
}

/* IRQ handler table keyed by pin bit index (0-15) */
static GpioIrqHandler *GpioIrqHandlers[16];
static void           *GpioIrqContexts[16];
static Gpio_t         *GpioIrqObjs[16];

void GpioMcuInit( Gpio_t *obj, PinNames pin, PinModes mode, PinConfigs config,
                  PinTypes type, uint32_t value )
{
    if( pin == NC )
    {
        return;
    }

    obj->pin      = pin;
    obj->port     = PinToPort( pin );
    obj->pinIndex = PinToMask( pin );

    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin = obj->pinIndex;

    switch( mode )
    {
        case PIN_INPUT:
            GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
            break;
        case PIN_OUTPUT:
            GPIO_InitStruct.Mode = ( config == PIN_OPEN_DRAIN )
                                   ? GPIO_MODE_OUTPUT_OD
                                   : GPIO_MODE_OUTPUT_PP;
            break;
        case PIN_ALTERNATE_FCT:
            GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
            break;
        case PIN_ANALOGIC:
            GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
            break;
        default:
            GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
            break;
    }

    switch( type )
    {
        case PIN_PULL_UP:   GPIO_InitStruct.Pull = GPIO_PULLUP;   break;
        case PIN_PULL_DOWN: GPIO_InitStruct.Pull = GPIO_PULLDOWN; break;
        default:            GPIO_InitStruct.Pull = GPIO_NOPULL;   break;
    }

    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init( (GPIO_TypeDef *)obj->port, &GPIO_InitStruct );

    if( mode == PIN_OUTPUT )
    {
        HAL_GPIO_WritePin( (GPIO_TypeDef *)obj->port, obj->pinIndex,
                           value ? GPIO_PIN_SET : GPIO_PIN_RESET );
    }
}

void GpioMcuSetContext( Gpio_t *obj, void *context )
{
    obj->Context = context;
}

void GpioMcuSetInterrupt( Gpio_t *obj, IrqModes irqMode, IrqPriorities irqPriority,
                          GpioIrqHandler *irqHandler )
{
    if( obj == NULL || obj->pin == NC )
    {
        return;
    }

    uint8_t bit = ( uint8_t )( obj->pin & 0x0F );
    GpioIrqHandlers[bit] = irqHandler;
    GpioIrqContexts[bit] = obj->Context;
    GpioIrqObjs[bit]     = obj;

    /* Re-initialise the pin to EXTI mode */
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin = obj->pinIndex;

    switch( irqMode )
    {
        case IRQ_RISING_EDGE:         GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;         break;
        case IRQ_FALLING_EDGE:        GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;        break;
        case IRQ_RISING_FALLING_EDGE: GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING; break;
        default:                      GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;         break;
    }

    switch( irqPriority )
    {
        case IRQ_VERY_LOW_PRIORITY:  GPIO_InitStruct.Pull = GPIO_NOPULL; break;
        case IRQ_LOW_PRIORITY:       GPIO_InitStruct.Pull = GPIO_NOPULL; break;
        default:                     GPIO_InitStruct.Pull = GPIO_NOPULL; break;
    }

    HAL_GPIO_Init( (GPIO_TypeDef *)obj->port, &GPIO_InitStruct );

    /* Priority: use NVIC with pre=1, sub=0 so DIO interrupts don't preempt each other */
    IRQn_Type irqn;
    switch( bit )
    {
        case 0:  irqn = EXTI0_IRQn;     break;
        case 1:  irqn = EXTI1_IRQn;     break;
        case 2:  irqn = EXTI2_IRQn;     break;
        case 3:  irqn = EXTI3_IRQn;     break;
        case 4:  irqn = EXTI4_IRQn;     break;
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:  irqn = EXTI9_5_IRQn;   break;
        default: irqn = EXTI15_10_IRQn; break;
    }

    HAL_NVIC_SetPriority( irqn, 1, 0 );
    HAL_NVIC_EnableIRQ( irqn );
}

void GpioMcuRemoveInterrupt( Gpio_t *obj )
{
    if( obj == NULL || obj->pin == NC )
    {
        return;
    }
    uint8_t bit = ( uint8_t )( obj->pin & 0x0F );
    GpioIrqHandlers[bit] = NULL;
    GpioIrqContexts[bit] = NULL;
    GpioIrqObjs[bit]     = NULL;
}

void GpioMcuWrite( Gpio_t *obj, uint32_t value )
{
    if( obj == NULL || obj->pin == NC )
    {
        return;
    }
    HAL_GPIO_WritePin( (GPIO_TypeDef *)obj->port, obj->pinIndex,
                       value ? GPIO_PIN_SET : GPIO_PIN_RESET );
}

void GpioMcuToggle( Gpio_t *obj )
{
    if( obj == NULL || obj->pin == NC )
    {
        return;
    }
    HAL_GPIO_TogglePin( (GPIO_TypeDef *)obj->port, obj->pinIndex );
}

uint32_t GpioMcuRead( Gpio_t *obj )
{
    if( obj == NULL || obj->pin == NC )
    {
        return 0;
    }
    return ( HAL_GPIO_ReadPin( (GPIO_TypeDef *)obj->port, obj->pinIndex ) == GPIO_PIN_SET ) ? 1 : 0;
}

/*!
 * Called from HAL_GPIO_EXTI_Callback – dispatches to the registered handler.
 */
void GpioMcuIrqHandler( uint16_t gpioPin )
{
    for( uint8_t bit = 0; bit < 16; bit++ )
    {
        if( ( gpioPin & ( 1U << bit ) ) && ( GpioIrqHandlers[bit] != NULL ) )
        {
            GpioIrqHandlers[bit]( GpioIrqContexts[bit] );
        }
    }
}
