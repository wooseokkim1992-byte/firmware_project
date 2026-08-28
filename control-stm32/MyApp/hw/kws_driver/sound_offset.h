#pragma once

#include <stdbool.h>
#include <stdint.h>

bool sound_offset_remove_window(const uint16_t *raw_samples,
                                int16_t *centered_samples,
                                uint32_t sample_count, uint16_t *dc_offset);
