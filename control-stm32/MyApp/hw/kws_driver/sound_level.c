#include "sound_level.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>

bool sound_level_calculate(const int16_t *centered_samples,
                           uint32_t sample_count, sound_level_data_t *result) {

  uint32_t abs_sum = 0U;
  uint64_t square_sum = 0U;
  uint32_t peak = 0U;

  if ((centered_samples == NULL) || (result == NULL) || (sample_count == 0U)) {
    return false;
  }

  for (uint32_t i = 0U; i < sample_count; i++) {
    int32_t sample = (int32_t)centered_samples[i];
    uint32_t abs_val =
        (sample >= 0) ? (uint32_t)sample : (uint32_t)(-sample);

    abs_sum += abs_val;
    square_sum += (uint64_t)abs_val * (uint64_t)abs_val;

    if (abs_val > peak) {
      peak = abs_val;
    }
  }

  result->mean_absolute = (float)abs_sum / (float)sample_count;
  result->rms = sqrtf((float)square_sum / (float)sample_count);
  result->peak = (uint16_t)peak;

  return true;
}
