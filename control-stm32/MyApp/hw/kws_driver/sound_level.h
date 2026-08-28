#pragma once

#include <stdbool.h>
#include <stdint.h>

#define SOUND_LEVEL_BASELINE_SIZE 20U

typedef struct {
  float mean_absolute;
  float rms;
  uint16_t peak;
} sound_level_data_t;

typedef enum {
  SOUND_LEVEL_STATE_NORMAL = 0,
  SOUND_LEVEL_STATE_WARNING
} sound_level_state_t;

/* Z-score thresholds and the number of consecutive windows required to
 * confirm each state transition. */
typedef struct {
  float rms_warning_z;
  float peak_warning_z;
  float rms_normal_z;
  float peak_normal_z;
  float stddev_epsilon;
  uint8_t warning_persistence;
  uint8_t normal_persistence;
} sound_level_detector_config_t;

/* Latest detector result. The Z-scores are calculated against the baseline
 * before the current sound window is inserted. */
typedef struct {
  sound_level_state_t current_state;
  float rms_mean;
  float rms_stddev;
  float peak_mean;
  float peak_stddev;
  float rms_z_score;
  float peak_z_score;
  float anomaly_score;
  uint8_t baseline_count;
  uint8_t warning_count;
  uint8_t normal_count;
  uint32_t update_count;
  uint32_t change_count;
  bool baseline_ready;
  bool configured;
  bool state_changed;
} sound_level_detector_data_t;

bool sound_level_calculate(const int16_t *centered_samples,
                           uint32_t sample_count,
                           sound_level_data_t *result);

void sound_level_detector_init(void);

bool sound_level_detector_set_config(
    const sound_level_detector_config_t *config);

/* Returns true only when NORMAL/WARNING actually changes. */
bool sound_level_detector_update(const sound_level_data_t *sound_level);

void sound_level_detector_get_data(sound_level_detector_data_t *result);

const char *sound_level_state_get_name(sound_level_state_t state);
