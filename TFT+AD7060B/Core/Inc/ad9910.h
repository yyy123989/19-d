#ifndef AD9910_H
#define AD9910_H

#include <stdint.h>

#define AD9910_DEFAULT_SYSCLK_HZ 1000000000UL
#define AD9910_MAX_AMPLITUDE    0x3FFFU

typedef enum {
  AD9910_RAM_WAVE_TRIANGLE = 0,
  AD9910_RAM_WAVE_SQUARE,
  AD9910_RAM_WAVE_SINE,
  AD9910_RAM_WAVE_SINC
} AD9910_RamWave;

typedef enum {
  AD9910_RAM_MODE_DIRECT = 0,
  AD9910_RAM_MODE_RAMP_UP,
  AD9910_RAM_MODE_BIDIRECTION,
  AD9910_RAM_MODE_CONT_BIDIRECTION,
  AD9910_RAM_MODE_CONT_RECIRC,
  AD9910_RAM_MODE_TRIANGLE_SQUARE
} AD9910_RamMode;

typedef enum {
  AD9910_RAM_TARGET_FREQUENCY = 0,
  AD9910_RAM_TARGET_AMPLITUDE,
  AD9910_RAM_TARGET_PHASE
} AD9910_RamTarget;

void AD9910_Init(void);
void AD9910_SetSingleTone(uint32_t freq_hz, uint16_t amplitude, uint16_t phase_deg);
void AD9910_SetDigitalRampFrequency(uint32_t lower_hz, uint32_t upper_hz,
                                    uint32_t rising_step_hz, uint32_t falling_step_hz,
                                    uint16_t rising_rate, uint16_t falling_rate);
void AD9910_SetDigitalRampDirection(uint8_t rising);
void AD9910_SetDigitalRampHold(uint8_t hold);
void AD9910_SetOsk(uint8_t enabled);
void AD9910_SetRamPlayback(AD9910_RamMode mode, AD9910_RamWave wave,
                           uint32_t freq_hz, uint16_t amplitude,
                           uint16_t duty_permille);
void AD9910_SetRamTablePlayback(AD9910_RamMode mode, AD9910_RamTarget target,
                                const uint32_t *words, uint16_t word_count);
void AD9910_SetTriangleWave(uint32_t freq_hz, uint16_t amplitude);
void AD9910_SelectProfile0(void);

#endif
