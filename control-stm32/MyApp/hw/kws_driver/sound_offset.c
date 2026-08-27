#include "sound_offset.h"

#include <stddef.h>

bool sound_offset_remove_window(const uint16_t *raw_samples,
                                int16_t *centered_samples,
                                uint32_t sample_count, uint16_t *dc_offset) {
  uint32_t sum = 0U;
  uint32_t offset;

  if ((raw_samples == NULL) || (centered_samples == NULL) ||
      (dc_offset == NULL) || (sample_count == 0U)) {
    return false;
  }

  /*
   * 1. 현재 측정 구간의 ADC 평균 계산
   *
   * 최대 합:
   * 4095 × 800 = 3,276,000
   *
   * uint32_t 범위에서 안전하다.
   */
  for (uint32_t i = 0U; i < sample_count; i++) {
    sum += raw_samples[i];
  }

  /*
   * 반올림을 적용한 정수 평균
   */
  offset = (sum + (sample_count / 2U)) / sample_count;

  /*
   * 2. 각 샘플에서 DC 오프셋 제거
   */
  for (uint32_t i = 0U; i < sample_count; i++) {
    centered_samples[i] = (int16_t)((int32_t)raw_samples[i] - (int32_t)offset);
  }

  *dc_offset = (uint16_t)offset;

  return true;
}
