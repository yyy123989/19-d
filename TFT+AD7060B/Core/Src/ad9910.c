#include "ad9910.h"

#include "ad9910_ram_tables.h"
#include "main.h"

#include <stddef.h>
#include <string.h>

#define AD9910_CFR1_REG      0x00U
#define AD9910_CFR2_REG      0x01U
#define AD9910_CFR3_REG      0x02U
#define AD9910_DR_LIMIT_REG  0x0BU
#define AD9910_DR_STEP_REG   0x0CU
#define AD9910_DR_RATE_REG   0x0DU
#define AD9910_PROFILE0_REG  0x0EU
#define AD9910_RAM_REG       0x16U
#define AD9910_RAM_POINTS    1024U
#define AD9910_RAM_END_ADDR  (AD9910_RAM_POINTS - 1U)
#define AD9910_RAM_STEP_MAX  0xFFFFFFUL
#define AD9910_RAM_STEP_DEFAULT 0x00FFFFUL
#define AD9910_CFR_BYTES     4U
#define AD9910_RAM_MODE_DIRECT_CODE     0x00U
#define AD9910_RAM_MODE_RAMP_UP_CODE    0x01U
#define AD9910_RAM_MODE_BIDIR_CODE      0x02U
#define AD9910_RAM_MODE_CONT_BIDIR_CODE 0x03U
#define AD9910_RAM_MODE_CONT_RECIRC_CODE 0x04U
#define AD9910_RAM_TARGET_FREQ_LOAD     0x00U
#define AD9910_RAM_TARGET_PHASE_LOAD    0x20U
#define AD9910_RAM_TARGET_AMP_LOAD      0x40U
#define AD9910_CFR1_RAM_ENABLE          0x80U
#define AD9910_SINE_Q14_FULL_SCALE      16384UL

static const uint8_t AD9910_CFR1_DEFAULT[AD9910_CFR_BYTES] = {0x00U, 0x40U, 0x00U, 0x00U};
static const uint8_t AD9910_CFR2_DEFAULT[AD9910_CFR_BYTES] = {0x01U, 0x00U, 0x00U, 0x00U};
static const uint8_t AD9910_CFR2_RAM_OR_SINGLE[AD9910_CFR_BYTES] = {0x01U, 0x40U, 0x08U, 0x20U};
static const uint8_t AD9910_CFR2_FREQ_DRG[AD9910_CFR_BYTES] = {0x00U, 0x48U, 0x08U, 0x20U};
static const uint8_t AD9910_CFR3_1GHZ_PLL[AD9910_CFR_BYTES] = {0x05U, 0x0FU, 0x41U, 0x50U};

/*
 * Wiring note: the AD9910 V1.2 reference code and some module docs use
 * PB3/PB5 for SCLK/SDIO. This project is actually wired through main.h as
 * DDS_SCLK=PB13 and DDS_SDIO=PB15, so the project pin definitions win.
 */

static uint32_t AD9910_ClampFrequency(uint32_t freq_hz)
{
    if (freq_hz > 420000000UL) {
        return 420000000UL;
    }
    return freq_hz;
}

static uint16_t AD9910_ClampAmplitude(uint16_t amplitude)
{
    if (amplitude > AD9910_MAX_AMPLITUDE) {
        return AD9910_MAX_AMPLITUDE;
    }
    return amplitude;
}

static void AD9910_ShortDelay(void)
{
    __NOP();
    __NOP();
    __NOP();
    __NOP();
}

static void AD9910_PinSet(GPIO_TypeDef *port, uint16_t pin)
{
    port->BSRR = pin;
}

static void AD9910_PinClr(GPIO_TypeDef *port, uint16_t pin)
{
    port->BSRR = ((uint32_t)pin << 16U);
}

static void AD9910_WriteByte(uint8_t data)
{
    uint8_t i;

    AD9910_PinClr(DDS_SCLK_GPIO_Port, DDS_SCLK_Pin);
    for (i = 0U; i < 8U; i++) {
        if ((data & 0x80U) != 0U) {
            AD9910_PinSet(DDS_SDIO_GPIO_Port, DDS_SDIO_Pin);
        } else {
            AD9910_PinClr(DDS_SDIO_GPIO_Port, DDS_SDIO_Pin);
        }

        AD9910_PinSet(DDS_SCLK_GPIO_Port, DDS_SCLK_Pin);
        AD9910_ShortDelay();
        AD9910_PinClr(DDS_SCLK_GPIO_Port, DDS_SCLK_Pin);
        data <<= 1U;
    }
}

static void AD9910_WriteRegister(uint8_t reg, const uint8_t *data, uint8_t len)
{
    uint8_t i;

    AD9910_PinClr(DDS_CS_GPIO_Port, DDS_CS_Pin);
    AD9910_WriteByte(reg);
    for (i = 0U; i < len; i++) {
        AD9910_WriteByte(data[i]);
    }
    AD9910_PinSet(DDS_CS_GPIO_Port, DDS_CS_Pin);
}

static void AD9910_PulseIoUpdate(void)
{
    AD9910_PinClr(DDS_IO_UPDATE_GPIO_Port, DDS_IO_UPDATE_Pin);
    AD9910_ShortDelay();
    AD9910_PinSet(DDS_IO_UPDATE_GPIO_Port, DDS_IO_UPDATE_Pin);
    AD9910_ShortDelay();
    AD9910_PinClr(DDS_IO_UPDATE_GPIO_Port, DDS_IO_UPDATE_Pin);
}

static uint32_t AD9910_FrequencyToFtw(uint32_t freq_hz)
{
    return (uint32_t)((((uint64_t)freq_hz) << 32U) / AD9910_DEFAULT_SYSCLK_HZ);
}

static uint16_t AD9910_PhaseToPow(uint16_t phase_deg)
{
    return (uint16_t)(((uint32_t)(phase_deg % 360U) * 65536UL) / 360UL);
}

static uint16_t AD9910_SineAbsQ14(uint16_t phase_deg)
{
    uint32_t phase;
    uint32_t product;
    uint32_t numerator;
    uint32_t denominator;

    phase = (uint32_t)(phase_deg % 360U);
    if (phase > 180UL) {
        phase = 360UL - phase;
    }

    product = phase * (180UL - phase);
    if (product == 0UL) {
        return 0U;
    }

    /* Bhaskara sine approximation: sin(x) ~= 4x(180-x)/(40500-x(180-x)). */
    numerator = 4UL * product * AD9910_SINE_Q14_FULL_SCALE;
    denominator = 40500UL - product;
    return (uint16_t)((numerator + (denominator / 2UL)) / denominator);
}

static void AD9910_U32ToBytes(uint32_t value, uint8_t *data)
{
    data[0] = (uint8_t)(value >> 24U);
    data[1] = (uint8_t)(value >> 16U);
    data[2] = (uint8_t)(value >> 8U);
    data[3] = (uint8_t)value;
}

static void AD9910_WriteRamWord(uint32_t data)
{
    AD9910_WriteByte((uint8_t)(data >> 24U));
    AD9910_WriteByte((uint8_t)(data >> 16U));
    AD9910_WriteByte((uint8_t)(data >> 8U));
    AD9910_WriteByte((uint8_t)data);
}

static uint32_t AD9910_RamStepRate(uint32_t freq_hz)
{
    uint64_t denom;
    uint64_t step;

    if (freq_hz == 0U) {
        return 1U;
    }

    denom = 4ULL * AD9910_RAM_POINTS * freq_hz;
    step = (((uint64_t)AD9910_DEFAULT_SYSCLK_HZ) + (denom / 2ULL)) / denom;
    if (step == 0ULL) {
        step = 1ULL;
    } else if (step > AD9910_RAM_STEP_MAX) {
        step = AD9910_RAM_STEP_MAX;
    }

    return (uint32_t)step;
}

static uint8_t AD9910_RamModeCode(AD9910_RamMode mode)
{
    switch (mode) {
        case AD9910_RAM_MODE_RAMP_UP:
            return AD9910_RAM_MODE_RAMP_UP_CODE;
        case AD9910_RAM_MODE_BIDIRECTION:
            return AD9910_RAM_MODE_BIDIR_CODE;
        case AD9910_RAM_MODE_CONT_BIDIRECTION:
            return AD9910_RAM_MODE_CONT_BIDIR_CODE;
        case AD9910_RAM_MODE_CONT_RECIRC:
        case AD9910_RAM_MODE_TRIANGLE_SQUARE:
            return AD9910_RAM_MODE_CONT_RECIRC_CODE;
        case AD9910_RAM_MODE_DIRECT:
        default:
            return AD9910_RAM_MODE_DIRECT_CODE;
    }
}

static uint8_t AD9910_RamTargetLoadByte(AD9910_RamTarget target)
{
    switch (target) {
        case AD9910_RAM_TARGET_PHASE:
            return AD9910_RAM_TARGET_PHASE_LOAD;
        case AD9910_RAM_TARGET_AMPLITUDE:
            return AD9910_RAM_TARGET_AMP_LOAD;
        case AD9910_RAM_TARGET_FREQUENCY:
        default:
            return AD9910_RAM_TARGET_FREQ_LOAD;
    }
}

static void AD9910_WriteRamProfile(uint32_t step_rate, uint16_t word_count, AD9910_RamMode mode)
{
    uint64_t profile;
    uint8_t data[8];
    uint8_t i;
    uint16_t end_addr;

    if (step_rate == 0U) {
        step_rate = 1U;
    } else if (step_rate > AD9910_RAM_STEP_MAX) {
        step_rate = AD9910_RAM_STEP_MAX;
    }

    if ((word_count == 0U) || (word_count > AD9910_RAM_POINTS)) {
        word_count = AD9910_RAM_POINTS;
    }
    end_addr = (uint16_t)(word_count - 1U);

    profile = (((uint64_t)step_rate) << 40U) |
              (((uint64_t)end_addr) << 30U) |
              (uint64_t)AD9910_RamModeCode(mode);

    for (i = 0U; i < sizeof(data); i++) {
        data[i] = (uint8_t)(profile >> (56U - (8U * i)));
    }

    AD9910_WriteRegister(AD9910_PROFILE0_REG, data, sizeof(data));
}

static void AD9910_WriteRamControl(AD9910_RamMode mode, AD9910_RamTarget target,
                                   uint16_t word_count, uint32_t step_rate)
{
    uint8_t cfr1_load[AD9910_CFR_BYTES];
    uint8_t target_load_byte = AD9910_RamTargetLoadByte(target);

    memcpy(cfr1_load, AD9910_CFR1_DEFAULT, sizeof(cfr1_load));
    cfr1_load[0] = target_load_byte;
    AD9910_WriteRegister(AD9910_CFR1_REG, cfr1_load, sizeof(cfr1_load));
    AD9910_WriteRegister(AD9910_CFR2_REG, AD9910_CFR2_RAM_OR_SINGLE, sizeof(AD9910_CFR2_RAM_OR_SINGLE));
    AD9910_WriteRamProfile(step_rate, word_count, mode);
}

static uint16_t AD9910_RamWaveSample(AD9910_RamWave wave, uint16_t index,
                                     uint16_t amplitude, uint16_t duty_permille)
{
    uint32_t sample;
    uint16_t half = AD9910_RAM_POINTS / 2U;
    uint16_t phase;

    amplitude = AD9910_ClampAmplitude(amplitude);
    if ((duty_permille < 50U) || (duty_permille > 950U)) {
        duty_permille = 500U;
    }

    switch (wave) {
        case AD9910_RAM_WAVE_SQUARE:
            sample = (((uint32_t)index * 1000UL) < ((uint32_t)AD9910_RAM_POINTS * duty_permille)) ?
                     amplitude : 0U;
            break;
        case AD9910_RAM_WAVE_SINE:
            /* Full-wave rectified sine envelope; RAM amplitude values are unipolar. */
            phase = (uint16_t)(((uint32_t)index * 360U) / AD9910_RAM_POINTS);
            sample = (((uint32_t)amplitude * AD9910_SineAbsQ14(phase)) +
                      (AD9910_SINE_Q14_FULL_SCALE / 2UL)) / AD9910_SINE_Q14_FULL_SCALE;
            break;
        case AD9910_RAM_WAVE_SINC:
        {
            /* sinc(x) = sin(pi*x)/(pi*x) — symmetrical about half, with side lobes */
            uint32_t x_q10;
            uint32_t pi_x_q10;
            uint16_t angle_deg;
            uint32_t sinc_q14;
            if (index > half) {
                x_q10 = ((uint32_t)(index - half) * 1024UL) / half;
            } else {
                x_q10 = ((uint32_t)(half - index) * 1024UL) / half;
            }
            if (x_q10 == 0U) {
                sample = amplitude;  /* sinc(0) = 1 */
            } else {
                pi_x_q10 = (x_q10 * 3217UL) / 1024UL;  /* pi*x in Q10 */
                angle_deg = (uint16_t)(((x_q10 * 180UL) + 512UL) / 1024UL);
                sinc_q14 = ((uint32_t)AD9910_SineAbsQ14(angle_deg) * 1024UL +
                            (pi_x_q10 / 2UL)) / pi_x_q10;
                sample = (((uint32_t)amplitude * sinc_q14) +
                          (AD9910_SINE_Q14_FULL_SCALE / 2UL)) / AD9910_SINE_Q14_FULL_SCALE;
            }
            break;
        }
        case AD9910_RAM_WAVE_TRIANGLE:
        default:
            phase = (index < half) ? index : (uint16_t)(AD9910_RAM_POINTS - 1U - index);
            sample = (((uint32_t)amplitude * phase) / (half - 1U));
            break;
    }

    if (sample > AD9910_MAX_AMPLITUDE) {
        sample = AD9910_MAX_AMPLITUDE;
    }
    return (uint16_t)sample;
}

void AD9910_Init(void)
{
    HAL_GPIO_WritePin(DDS_PWRDN_GPIO_Port, DDS_PWRDN_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DDS_OSK_GPIO_Port, DDS_OSK_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DDS_DRCTL_GPIO_Port, DDS_DRCTL_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DDS_DRHOLD_GPIO_Port, DDS_DRHOLD_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DDS_PROFILE0_GPIO_Port, DDS_PROFILE0_Pin, GPIO_PIN_RESET);

    HAL_GPIO_WritePin(DDS_CS_GPIO_Port, DDS_CS_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(DDS_SCLK_GPIO_Port, DDS_SCLK_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DDS_SDIO_GPIO_Port, DDS_SDIO_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(DDS_IO_UPDATE_GPIO_Port, DDS_IO_UPDATE_Pin, GPIO_PIN_RESET);

    HAL_GPIO_WritePin(DDS_RESET_GPIO_Port, DDS_RESET_Pin, GPIO_PIN_SET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(DDS_RESET_GPIO_Port, DDS_RESET_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);

    AD9910_WriteRegister(AD9910_CFR1_REG, AD9910_CFR1_DEFAULT, sizeof(AD9910_CFR1_DEFAULT));
    AD9910_WriteRegister(AD9910_CFR2_REG, AD9910_CFR2_DEFAULT, sizeof(AD9910_CFR2_DEFAULT));
    AD9910_WriteRegister(AD9910_CFR3_REG, AD9910_CFR3_1GHZ_PLL, sizeof(AD9910_CFR3_1GHZ_PLL));
    AD9910_PulseIoUpdate();
    HAL_Delay(20);
}

void AD9910_SetSingleTone(uint32_t freq_hz, uint16_t amplitude, uint16_t phase_deg)
{
    uint32_t ftw;
    uint16_t pow;
    uint8_t data[8];

    freq_hz = AD9910_ClampFrequency(freq_hz);
    amplitude = AD9910_ClampAmplitude(amplitude);

    ftw = AD9910_FrequencyToFtw(freq_hz);
    pow = AD9910_PhaseToPow(phase_deg);

    data[0] = (uint8_t)(amplitude >> 8U);
    data[1] = (uint8_t)amplitude;
    data[2] = (uint8_t)(pow >> 8U);
    data[3] = (uint8_t)pow;
    data[4] = (uint8_t)(ftw >> 24U);
    data[5] = (uint8_t)(ftw >> 16U);
    data[6] = (uint8_t)(ftw >> 8U);
    data[7] = (uint8_t)ftw;

    AD9910_WriteRegister(AD9910_CFR1_REG, AD9910_CFR1_DEFAULT, sizeof(AD9910_CFR1_DEFAULT));
    AD9910_WriteRegister(AD9910_CFR2_REG, AD9910_CFR2_RAM_OR_SINGLE, sizeof(AD9910_CFR2_RAM_OR_SINGLE));
    AD9910_WriteRegister(AD9910_PROFILE0_REG, data, sizeof(data));
    AD9910_PulseIoUpdate();
}

void AD9910_SetDigitalRampFrequency(uint32_t lower_hz, uint32_t upper_hz,
                                    uint32_t rising_step_hz, uint32_t falling_step_hz,
                                    uint16_t rising_rate, uint16_t falling_rate)
{
    uint32_t temp;
    uint8_t data[8];

    lower_hz = AD9910_ClampFrequency(lower_hz);
    upper_hz = AD9910_ClampFrequency(upper_hz);

    if (upper_hz < lower_hz) {
        temp = upper_hz;
        upper_hz = lower_hz;
        lower_hz = temp;
    }

    if (rising_step_hz == 0U) {
        rising_step_hz = 1U;
    }
    if (falling_step_hz == 0U) {
        falling_step_hz = rising_step_hz;
    }

    HAL_GPIO_WritePin(DDS_DRHOLD_GPIO_Port, DDS_DRHOLD_Pin, GPIO_PIN_RESET);
    AD9910_SetDigitalRampDirection(0U);

    AD9910_WriteRegister(AD9910_CFR1_REG, AD9910_CFR1_DEFAULT, sizeof(AD9910_CFR1_DEFAULT));
    AD9910_WriteRegister(AD9910_CFR2_REG, AD9910_CFR2_FREQ_DRG, sizeof(AD9910_CFR2_FREQ_DRG));
    AD9910_PulseIoUpdate();

    AD9910_U32ToBytes(AD9910_FrequencyToFtw(upper_hz), &data[0]);
    AD9910_U32ToBytes(AD9910_FrequencyToFtw(lower_hz), &data[4]);
    AD9910_WriteRegister(AD9910_DR_LIMIT_REG, data, sizeof(data));

    AD9910_U32ToBytes(AD9910_FrequencyToFtw(falling_step_hz), &data[0]);
    AD9910_U32ToBytes(AD9910_FrequencyToFtw(rising_step_hz), &data[4]);
    AD9910_WriteRegister(AD9910_DR_STEP_REG, data, sizeof(data));

    data[0] = (uint8_t)(falling_rate >> 8U);
    data[1] = (uint8_t)falling_rate;
    data[2] = (uint8_t)(rising_rate >> 8U);
    data[3] = (uint8_t)rising_rate;
    AD9910_WriteRegister(AD9910_DR_RATE_REG, data, 4U);

    AD9910_PulseIoUpdate();
    AD9910_ShortDelay();
    AD9910_SetDigitalRampDirection(1U);
}

void AD9910_SetDigitalRampDirection(uint8_t rising)
{
    HAL_GPIO_WritePin(DDS_DRCTL_GPIO_Port, DDS_DRCTL_Pin,
                      (rising != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void AD9910_SetDigitalRampHold(uint8_t hold)
{
    HAL_GPIO_WritePin(DDS_DRHOLD_GPIO_Port, DDS_DRHOLD_Pin,
                      (hold != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void AD9910_SetOsk(uint8_t enabled)
{
    HAL_GPIO_WritePin(DDS_OSK_GPIO_Port, DDS_OSK_Pin,
                      (enabled != 0U) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static void AD9910_SetRamTablePlaybackWithStep(AD9910_RamMode mode, AD9910_RamTarget target,
                                               const uint32_t *words, uint16_t word_count,
                                               uint32_t step_rate)
{
    uint8_t cfr1_run[AD9910_CFR_BYTES];
    uint16_t i;

    if ((words == NULL) || (word_count == 0U)) {
        return;
    }
    if (word_count > AD9910_RAM_POINTS) {
        word_count = AD9910_RAM_POINTS;
    }

    memcpy(cfr1_run, AD9910_CFR1_DEFAULT, sizeof(cfr1_run));
    cfr1_run[0] = (uint8_t)(AD9910_CFR1_RAM_ENABLE | AD9910_RamTargetLoadByte(target));
    AD9910_SelectProfile0();
    AD9910_WriteRamControl(mode, target, word_count, step_rate);
    AD9910_PulseIoUpdate();

    AD9910_PinClr(DDS_CS_GPIO_Port, DDS_CS_Pin);
    AD9910_WriteByte(AD9910_RAM_REG);
    for (i = 0U; i < word_count; i++) {
        AD9910_WriteRamWord(words[i]);
    }
    AD9910_PinSet(DDS_CS_GPIO_Port, DDS_CS_Pin);

    AD9910_WriteRegister(AD9910_CFR1_REG, cfr1_run, sizeof(cfr1_run));
    AD9910_PulseIoUpdate();
}

void AD9910_SetRamTablePlayback(AD9910_RamMode mode, AD9910_RamTarget target,
                                const uint32_t *words, uint16_t word_count)
{
    AD9910_SetRamTablePlaybackWithStep(mode, target, words, word_count, AD9910_RAM_STEP_DEFAULT);
}

void AD9910_SetRamPlayback(AD9910_RamMode mode, AD9910_RamWave wave,
                           uint32_t freq_hz, uint16_t amplitude,
                           uint16_t duty_permille)
{
    /* Generate simple RAM waveforms while writing RAM to avoid extra const tables. */
    uint32_t step_rate;
    uint8_t cfr1_run[AD9910_CFR_BYTES];
    uint16_t i;
    uint32_t ram_word;

    step_rate = AD9910_RamStepRate(AD9910_ClampFrequency(freq_hz));
    amplitude = AD9910_ClampAmplitude(amplitude);

    memcpy(cfr1_run, AD9910_CFR1_DEFAULT, sizeof(cfr1_run));
    cfr1_run[0] = (uint8_t)(AD9910_CFR1_RAM_ENABLE | AD9910_RAM_TARGET_AMP_LOAD);

    AD9910_SelectProfile0();
    AD9910_WriteRamControl(mode, AD9910_RAM_TARGET_AMPLITUDE, AD9910_RAM_POINTS, step_rate);
    AD9910_PulseIoUpdate();

    AD9910_PinClr(DDS_CS_GPIO_Port, DDS_CS_Pin);
    AD9910_WriteByte(AD9910_RAM_REG);
    for (i = 0U; i < AD9910_RAM_POINTS; i++) {
        ram_word = ((uint32_t)AD9910_RamWaveSample(wave, i, amplitude, duty_permille)) << 18U;
        AD9910_WriteRamWord(ram_word);
    }
    AD9910_PinSet(DDS_CS_GPIO_Port, DDS_CS_Pin);

    AD9910_WriteRegister(AD9910_CFR1_REG, cfr1_run, sizeof(cfr1_run));
    AD9910_PulseIoUpdate();
}

void AD9910_SetTriangleWave(uint32_t freq_hz, uint16_t amplitude)
{
    AD9910_SetRamPlayback(AD9910_RAM_MODE_TRIANGLE_SQUARE,
                          AD9910_RAM_WAVE_TRIANGLE,
                          freq_hz,
                          amplitude,
                          500U);
}

void AD9910_SelectProfile0(void)
{
    /* Profile1/Profile2 are strapped low in this project, so the driver exposes Profile0 only. */
    HAL_GPIO_WritePin(DDS_PROFILE0_GPIO_Port, DDS_PROFILE0_Pin,
                      GPIO_PIN_RESET);
    AD9910_PulseIoUpdate();
}
