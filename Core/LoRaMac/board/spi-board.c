/*!
 * \file      spi-board.c
 *
 * \brief     SPI board-level driver for STM32F401 (SPI1 / HAL).
 *
 *            SpiInit configures the SPI object's SpiId field;
 *            SpiInOut performs a full-duplex byte transfer via hspi1 with
 *            software NSS (the caller drives NSS separately via GpioWrite).
 */
#include "stm32f4xx_hal.h"
#include "spi.h"

extern SPI_HandleTypeDef hspi1;

void SpiInit( Spi_t *obj, SpiId_t spiId, PinNames mosi, PinNames miso,
              PinNames sclk, PinNames nss )
{
    obj->SpiId = spiId;
    /* SPI1 peripheral is already initialised by MX_SPI1_Init() in main.c.
     * GPIO pins are configured by the HAL MSP callback.
     * No further action needed here. */
    (void)mosi; (void)miso; (void)sclk; (void)nss;
}

void SpiDeInit( Spi_t *obj )
{
    (void)obj;
}

void SpiFormat( Spi_t *obj, int8_t bits, int8_t cpol, int8_t cpha, int8_t slave )
{
    (void)obj; (void)bits; (void)cpol; (void)cpha; (void)slave;
}

void SpiFrequency( Spi_t *obj, uint32_t hz )
{
    (void)obj; (void)hz;
}

uint16_t SpiInOut( Spi_t *obj, uint16_t outData )
{
    uint8_t txByte = ( uint8_t )outData;
    uint8_t rxByte = 0;
    HAL_StatusTypeDef hs;
    (void)obj;

    hs = HAL_SPI_TransmitReceive( &hspi1, &txByte, &rxByte, 1, 10U );
    if( hs != HAL_OK )
    {
        return 0xFFU;
    }
    return ( uint16_t )rxByte;
}
