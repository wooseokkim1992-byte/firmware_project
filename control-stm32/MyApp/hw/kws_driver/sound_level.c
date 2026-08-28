#include "sound_level.h"

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct {
  float rms[SOUND_LEVEL_BASELINE_SIZE];
  float peak[SOUND_LEVEL_BASELINE_SIZE];
  double rms_sum;
  double rms_square_sum;
  double peak_sum;
  double peak_square_sum;
  uint8_t head;
  uint8_t count;
} sound_level_baseline_t;

static sound_level_baseline_t baseline = {0};
static sound_level_detector_config_t detector_config = {0};
static sound_level_detector_data_t detector_data = {0};

static void sound_level_baseline_update_statistics(void) {
  double rms_variance = 0.0;
  double peak_variance = 0.0;
  double count = (double)baseline.count;

  detector_data.baseline_count = baseline.count;
  detector_data.baseline_ready =
      (baseline.count == SOUND_LEVEL_BASELINE_SIZE);

  if (baseline.count == 0U) {
    detector_data.rms_mean = 0.0f;
    detector_data.rms_stddev = 0.0f;
    detector_data.peak_mean = 0.0f;
    detector_data.peak_stddev = 0.0f;
    return;
  }

  detector_data.rms_mean = (float)(baseline.rms_sum / count);
  detector_data.peak_mean = (float)(baseline.peak_sum / count);

  if (baseline.count < 2U) {
    detector_data.rms_stddev = 0.0f;
    detector_data.peak_stddev = 0.0f;
    return;
  }

  /* Sample variance from the rolling sum and square sum. */
  rms_variance =
      (baseline.rms_square_sum -
       ((baseline.rms_sum * baseline.rms_sum) / count)) /
      (count - 1.0);
  peak_variance =
      (baseline.peak_square_sum -
       ((baseline.peak_sum * baseline.peak_sum) / count)) /
      (count - 1.0);

  /* Floating-point rounding can leave a very small negative variance. */
  if (rms_variance < 0.0) {
    rms_variance = 0.0;
  }
  if (peak_variance < 0.0) {
    peak_variance = 0.0;
  }

  detector_data.rms_stddev = sqrtf((float)rms_variance);
  detector_data.peak_stddev = sqrtf((float)peak_variance);
}

static void sound_level_baseline_push(float rms, float peak) {
  uint8_t index = baseline.head;

  if (baseline.count == SOUND_LEVEL_BASELINE_SIZE) {
    double old_rms = (double)baseline.rms[index];
    double old_peak = (double)baseline.peak[index];

    baseline.rms_sum -= old_rms;
    baseline.rms_square_sum -= old_rms * old_rms;
    baseline.peak_sum -= old_peak;
    baseline.peak_square_sum -= old_peak * old_peak;
  } else {
    baseline.count++;
  }

  baseline.rms[index] = rms;
  baseline.peak[index] = peak;
  baseline.rms_sum += (double)rms;
  baseline.rms_square_sum += (double)rms * (double)rms;
  baseline.peak_sum += (double)peak;
  baseline.peak_square_sum += (double)peak * (double)peak;

  baseline.head = (uint8_t)((index + 1U) % SOUND_LEVEL_BASELINE_SIZE);
  sound_level_baseline_update_statistics();
}

static float sound_level_z_score(float value, float mean, float stddev) {
  float denominator = stddev;

  if (denominator < detector_config.stddev_epsilon) {
    denominator = detector_config.stddev_epsilon;
  }

  return (value - mean) / denominator;
}

bool sound_level_calculate(const int16_t *centered_samples,
                           uint32_t sample_count,
                           sound_level_data_t *result) {
  uint32_t abs_sum = 0U;
  uint64_t square_sum = 0U;
  uint32_t peak = 0U;

  if ((centered_samples == NULL) || (result == NULL) || (sample_count == 0U)) {
    return false;
  }

  for (uint32_t i = 0U; i < sample_count; i++) {
    int32_t sample = (int32_t)centered_samples[i];
    uint32_t abs_value =
        (sample >= 0) ? (uint32_t)sample : (uint32_t)(-sample);

    abs_sum += abs_value;
    square_sum += (uint64_t)abs_value * (uint64_t)abs_value;

    if (abs_value > peak) {
      peak = abs_value;
    }
  }

  result->mean_absolute = (float)abs_sum / (float)sample_count;
  result->rms = sqrtf((float)square_sum / (float)sample_count);
  result->peak = (uint16_t)peak;

  return true;
}

void sound_level_detector_init(void) {
  memset(&baseline, 0, sizeof(baseline));
  memset(&detector_config, 0, sizeof(detector_config));
  memset(&detector_data, 0, sizeof(detector_data));
  detector_data.current_state = SOUND_LEVEL_STATE_NORMAL;
}

bool sound_level_detector_set_config(
    const sound_level_detector_config_t *config) {
  if (config == NULL) {
    return false;
  }

  if ((config->rms_warning_z <= 0.0f) ||
      (config->peak_warning_z <= 0.0f) ||
      (config->rms_normal_z < 0.0f) ||
      (config->peak_normal_z < 0.0f) ||
      (config->rms_normal_z >= config->rms_warning_z) ||
      (config->peak_normal_z >= config->peak_warning_z) ||
      (config->stddev_epsilon <= 0.0f) ||
      (config->warning_persistence == 0U) ||
      (config->normal_persistence == 0U)) {
    return false;
  }

  detector_config = *config;
  detector_data.configured = true;
  detector_data.warning_count = 0U;
  detector_data.normal_count = 0U;
  detector_data.state_changed = false;
  return true;
}

bool sound_level_detector_update(const sound_level_data_t *sound_level) {
  bool warning_candidate;
  bool normal_candidate;
  bool baseline_candidate;
  float peak;

  detector_data.state_changed = false;

  if ((sound_level == NULL) || !detector_data.configured ||
      (sound_level->rms < 0.0f)) {
    return false;
  }

  detector_data.update_count++;
  peak = (float)sound_level->peak;

  /* The first 20 windows form the initial NORMAL baseline. */
  if (!detector_data.baseline_ready) {
    sound_level_baseline_push(sound_level->rms, peak);
    detector_data.rms_z_score = 0.0f;
    detector_data.peak_z_score = 0.0f;
    detector_data.anomaly_score = 0.0f;
    return false;
  }

  /* Calculate both Z-scores before deciding whether this window belongs in
   * the baseline. */
  detector_data.rms_z_score =
      sound_level_z_score(sound_level->rms, detector_data.rms_mean,
                          detector_data.rms_stddev);
  detector_data.peak_z_score =
      sound_level_z_score(peak, detector_data.peak_mean,
                          detector_data.peak_stddev);
  detector_data.anomaly_score =
      fmaxf(detector_data.rms_z_score, detector_data.peak_z_score);

  warning_candidate =
      (detector_data.rms_z_score >= detector_config.rms_warning_z) ||
      (detector_data.peak_z_score >= detector_config.peak_warning_z);
  normal_candidate =
      (fabsf(detector_data.rms_z_score) <= detector_config.rms_normal_z) &&
      (fabsf(detector_data.peak_z_score) <= detector_config.peak_normal_z);
  baseline_candidate =
      (fabsf(detector_data.rms_z_score) < detector_config.rms_warning_z) &&
      (fabsf(detector_data.peak_z_score) < detector_config.peak_warning_z);

  if (detector_data.current_state == SOUND_LEVEL_STATE_NORMAL) {
    detector_data.normal_count = 0U;

    if (warning_candidate) {
      if (detector_data.warning_count < UINT8_MAX) {
        detector_data.warning_count++;
      }

      if (detector_data.warning_count >=
          detector_config.warning_persistence) {
        detector_data.current_state = SOUND_LEVEL_STATE_WARNING;
        detector_data.warning_count = 0U;
        detector_data.change_count++;
        detector_data.state_changed = true;
      }

      /* An abnormal candidate must not move the NORMAL baseline. */
      return detector_data.state_changed;
    }

    detector_data.warning_count = 0U;

    /* A very large negative outlier is not a WARNING, but it is also not
     * allowed to contaminate the NORMAL baseline. */
    if (baseline_candidate) {
      sound_level_baseline_push(sound_level->rms, peak);
    }

    return false;
  }

  /* While WARNING is active, freeze the baseline. A return to its vicinity
   * (Z ~= 0), not a large negative Z, is the recovery condition. */
  detector_data.warning_count = 0U;

  if (normal_candidate) {
    if (detector_data.normal_count < UINT8_MAX) {
      detector_data.normal_count++;
    }

    if (detector_data.normal_count >= detector_config.normal_persistence) {
      detector_data.current_state = SOUND_LEVEL_STATE_NORMAL;
      detector_data.normal_count = 0U;
      detector_data.change_count++;
      detector_data.state_changed = true;
      sound_level_baseline_push(sound_level->rms, peak);
    }
  } else {
    detector_data.normal_count = 0U;
  }

  return detector_data.state_changed;
}

void sound_level_detector_get_data(sound_level_detector_data_t *result) {
  if (result != NULL) {
    *result = detector_data;
  }
}

const char *sound_level_state_get_name(sound_level_state_t state) {
  switch (state) {
  case SOUND_LEVEL_STATE_NORMAL:
    return "NORMAL";
  case SOUND_LEVEL_STATE_WARNING:
    return "WARNING";
  default:
    return "UNKNOWN";
  }
}
