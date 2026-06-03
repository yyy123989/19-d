#include "ad7606b.h"

#include "main.h"
#include "spi.h"

#include <stddef.h>
#include <stdio.h>

/*
 * AD7606B 当前按串行模式使用：
 * 1. TIM3 固定周期拉 CONVST，触发外部 ADC 同步转换。
 * 2. BUSY 下降沿说明转换完成，在 EXTI1 中读取一次 8 通道 SPI 帧。
 * 3. 主循环只拿 ad_samples 快照显示/发送，LCD 和串口不会反过来决定采样节拍。
 */
#define AD7606B_TIMEOUT_COUNT 2000U /* 按 SYSCLK=168MHz 估算，系统时钟变化时要同步缩放 */
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

    /* FRSTDATA 置位后再开始读，保证第一个 SPI word 对应 CH1。 */
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

    /* 读帧前清一次 RX 状态，避免上一次残留字节污染当前 8 通道数据。 */
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
    /* 先把片选、写控制、待机脚放到安全状态，再配置模式脚和复位 ADC。 */
    HAL_GPIO_WritePin(AD_CS_GPIO_Port, AD_CS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(AD_WR_GPIO_Port, AD_WR_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(AD_STBY_GPIO_Port, AD_STBY_Pin, GPIO_PIN_SET);

    /* PAR/SER 拉高选择串行模式，PA5/PA6 分别作为 SPI1 SCLK/MISO。 */
    HAL_GPIO_WritePin(AD_PAR_SER_GPIO_Port, AD_PAR_SER_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(AD_RANGE_GPIO_Port, AD_RANGE_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(AD_OS0_GPIO_Port, AD_OS0_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(AD_OS1_GPIO_Port, AD_OS1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(AD_OS2_GPIO_Port, AD_OS2_Pin, GPIO_PIN_RESET);

    /* 复位期间 BUSY 可能跳变，先屏蔽 EXTI，复位结束后再打开。 */
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
    /* CONVST 低脉冲触发转换，低电平宽度由 NOP 保底，避免被优化成过窄脉冲。 */
    AD_CONVST_GPIO_Port->BSRR = ((uint32_t)AD_CONVST_Pin << 16U);
    AD7606B_DelayConvstLow();
    AD_CONVST_GPIO_Port->BSRR = AD_CONVST_Pin;
}

int32_t AD7606B_RawToMillivolts(int16_t raw)
{
    int32_t code = raw;
    int32_t magnitude;
    int32_t millivolts;

    /* 当前 RANGE=0，对应 +/-5V 量程，原始补码按满量程 32768 换算成 mV。 */
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

    /* BUSY 已经为低表示转换完成，整个 8 通道串行帧期间 CS 必须保持低电平。 */
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

    /* 轮询版本只作为兼容接口保留，实时采样优先走 BUSY 下降沿中断。 */
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
