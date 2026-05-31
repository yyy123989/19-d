#include "ad7606b.h"

#include "main.h"
#include "spi.h"

#include <stddef.h>
#include <stdio.h>

#define AD7606B_TIMEOUT_COUNT 2000U /* assumes SYSCLK=168MHz; scale proportionally if clock changes */
#define AD7606B_SPI_TIMEOUT_COUNT 1000U
#define AD7606B_CODE_FULL_SCALE 32768L
#define AD7606B_CONVST_LOW_NOP_COUNT 10U

static void AD7606B_DelayConvstLow(void)
{
    uint8_t i;

    for (i = 0U; i < AD7606B_CONVST_LOW_NOP_COUNT; i++) {
        __NOP();
    }
}

static uint8_t AD7606B_WaitFirstData(void)
{
    uint32_t timeout = AD7606B_TIMEOUT_COUNT;

    while (((AD_FRSTDATA_GPIO_Port->IDR & AD_FRSTDATA_Pin) == 0U) && (timeout > 0U)) {
        timeout--;
    }

    return (timeout > 0U) ? 1U : 0U;
}

static uint8_t AD7606B_SpiTransferWord(uint16_t *value)
{
    uint32_t timeout;

    if (value == NULL) {
        return 0U;
    }

    timeout = AD7606B_SPI_TIMEOUT_COUNT;
    while (((SPI1->SR & SPI_SR_TXE) == 0U) && (timeout > 0U)) {
        timeout--;
    }
    if (timeout == 0U) {
        return 0U;
    }

    SPI1->DR = 0x0000U;

    timeout = AD7606B_SPI_TIMEOUT_COUNT;
    while (((SPI1->SR & SPI_SR_RXNE) == 0U) && (timeout > 0U)) {
        timeout--;
    }
    if (timeout == 0U) {
        return 0U;
    }

    *value = (uint16_t)SPI1->DR;
    return 1U;
}

static uint8_t AD7606B_ReadSerialFrame(int16_t samples[AD7606B_CHANNEL_COUNT])
{
    uint16_t words[AD7606B_CHANNEL_COUNT];
    uint8_t i;
    uint32_t timeout;

    if (samples == NULL) {
        return 0U;
    }

    /* Flush SPI RX before starting the frame. */
    (void)SPI1->DR;
    (void)SPI1->SR;

    for (i = 0U; i < AD7606B_CHANNEL_COUNT; i++) {
        if (AD7606B_SpiTransferWord(&words[i]) == 0U) {
            return 0U;
        }
    }

    timeout = AD7606B_SPI_TIMEOUT_COUNT;
    while (((SPI1->SR & SPI_SR_BSY) != 0U) && (timeout > 0U)) {
        timeout--;
    }
    if (timeout == 0U) {
        return 0U;
    }

    for (i = 0U; i < AD7606B_CHANNEL_COUNT; i++) {
        samples[i] = (int16_t)words[i];
    }

    return 1U;
}

void AD7606B_Init(void)
{
    HAL_GPIO_WritePin(AD_CS_GPIO_Port, AD_CS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(AD_WR_GPIO_Port, AD_WR_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(AD_STBY_GPIO_Port, AD_STBY_Pin, GPIO_PIN_SET);

    /* PAR/SER high selects serial mode; PA5/PA6 are SPI1 SCLK/MISO for AD7606B. */
    HAL_GPIO_WritePin(AD_PAR_SER_GPIO_Port, AD_PAR_SER_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(AD_RANGE_GPIO_Port, AD_RANGE_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(AD_OS0_GPIO_Port, AD_OS0_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(AD_OS1_GPIO_Port, AD_OS1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(AD_OS2_GPIO_Port, AD_OS2_Pin, GPIO_PIN_RESET);

    HAL_NVIC_DisableIRQ(EXTI1_IRQn);
    HAL_GPIO_WritePin(AD_RESET_GPIO_Port, AD_RESET_Pin, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(AD_RESET_GPIO_Port, AD_RESET_Pin, GPIO_PIN_RESET);
    HAL_Delay(1);
    __HAL_GPIO_EXTI_CLEAR_IT(AD_BUSY_Pin);
    HAL_NVIC_EnableIRQ(EXTI1_IRQn);

    HAL_GPIO_WritePin(AD_CONVST_GPIO_Port, AD_CONVST_Pin, GPIO_PIN_SET);

    SPI1->CR1 |= SPI_CR1_SPE;
}

void AD7606B_StartConversion(void)
{
    AD_CONVST_GPIO_Port->BSRR = ((uint32_t)AD_CONVST_Pin << 16U);
    AD7606B_DelayConvstLow();
    AD_CONVST_GPIO_Port->BSRR = AD_CONVST_Pin;
}

int32_t AD7606B_RawToMillivolts(int16_t raw)
{
    int32_t code = raw;
    int32_t magnitude;
    int32_t millivolts;

    if (code < 0) {
        magnitude = -code;
        millivolts = (magnitude * AD7606B_RANGE_MV + (AD7606B_CODE_FULL_SCALE / 2L)) /
                     AD7606B_CODE_FULL_SCALE;
        return -millivolts;
    }

    return (code * AD7606B_RANGE_MV + (AD7606B_CODE_FULL_SCALE / 2L)) /
           AD7606B_CODE_FULL_SCALE;
}

uint32_t AD7606B_FormatVoltage(char *buffer, uint32_t size, int16_t raw, uint8_t append_unit)
{
    int32_t millivolts = AD7606B_RawToMillivolts(raw);
    uint32_t magnitude;
    int written;

    if ((buffer == NULL) || (size == 0U)) {
        return 0U;
    }

    if (millivolts < 0) {
        magnitude = (uint32_t)(-millivolts);
        written = snprintf(buffer, size, "-%lu.%03lu%s",
                           (unsigned long)(magnitude / 1000UL),
                           (unsigned long)(magnitude % 1000UL),
                           (append_unit != 0U) ? "V" : "");
    } else {
        magnitude = (uint32_t)millivolts;
        written = snprintf(buffer, size, "+%lu.%03lu%s",
                           (unsigned long)(magnitude / 1000UL),
                           (unsigned long)(magnitude % 1000UL),
                           (append_unit != 0U) ? "V" : "");
    }

    if (written <= 0) {
        return 0U;
    }
    if ((uint32_t)written >= size) {
        return size - 1U;
    }

    return (uint32_t)written;
}

uint8_t AD7606B_ReadChannelsReady(int16_t samples[AD7606B_CHANNEL_COUNT])
{
    if (samples == NULL) {
        return 0U;
    }
    if ((AD_BUSY_GPIO_Port->IDR & AD_BUSY_Pin) != 0U) {
        return 0U;
    }

    /* BUSY falling means conversion is complete; keep CS low for one serial frame. */
    AD_CS_GPIO_Port->BSRR = ((uint32_t)AD_CS_Pin << 16U);
    if (AD7606B_WaitFirstData() == 0U) {
        AD_CS_GPIO_Port->BSRR = AD_CS_Pin;
        return 0U;
    }
    if (AD7606B_ReadSerialFrame(samples) == 0U) {
        AD_CS_GPIO_Port->BSRR = AD_CS_Pin;
        return 0U;
    }
    AD_CS_GPIO_Port->BSRR = AD_CS_Pin;

    return 1U;
}

uint8_t AD7606B_ReadChannels(int16_t samples[AD7606B_CHANNEL_COUNT])
{
    uint32_t timeout = AD7606B_TIMEOUT_COUNT;

    while (((AD_BUSY_GPIO_Port->IDR & AD_BUSY_Pin) == 0U) && (timeout > 0U)) {
        timeout--;
    }

    if (timeout == 0U) {
        return 0U;
    }

    if (samples == NULL) {
        return 0U;
    }

    timeout = AD7606B_TIMEOUT_COUNT;
    while (((AD_BUSY_GPIO_Port->IDR & AD_BUSY_Pin) != 0U) && (timeout > 0U)) {
        timeout--;
    }

    if (timeout == 0U) {
        return 0U;
    }

    return AD7606B_ReadChannelsReady(samples);
}
