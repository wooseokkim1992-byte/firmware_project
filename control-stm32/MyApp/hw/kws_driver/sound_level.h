#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  float mean_absolute;
  float rms;
  uint16_t peak;
} sound_level_data_t;

bool sound_level_calculate(const int16_t *centered_samples,
                           uint32_t sample_count, sound_level_data_t *result);
