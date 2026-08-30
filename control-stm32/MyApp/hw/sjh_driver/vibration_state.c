#include "vibration_state.h"

#include <string.h>

/*
 * ============================================================
 * 상태 판정 설정
 * ============================================================
 */
static vibration_state_config_t state_config = {0};

/*
 * ============================================================
 * 상태 판정 결과
 * ============================================================
 */
static vibration_state_data_t state_data = {0};

/*
 * ============================================================
 * 내부 상태 판정 함수
 * ============================================================
 *
 * 현재 상태와 진동값을 이용하여
 * 다음 후보 상태를 결정한다.
 *
 * 여기서는 아직 상태를 바로 변경하지 않는다.
 *
 * 실제 변경 여부는
 * persistence_count 검사 이후 결정한다.
 */
static vibration_state_t vibration_state_determine(float vibration_value) {
  /*
   * ========================================================
   * 현재 NORMAL
   * ========================================================
   */
  if (state_data.current_state == VIBRATION_STATE_NORMAL) {
    /*
     * 한 번의 Update에서는 한 단계만 이동한다.
     * DANGER 기준을 넘는 큰 진동이어도 먼저 WARNING으로 진입하고,
     * 다음 Update에서 계속 큰 진동이 감지되면 DANGER로 진입한다.
     */
    if (vibration_value >= state_config.warning_threshold) {
      return VIBRATION_STATE_WARNING;
    }

    return VIBRATION_STATE_NORMAL;
  }

  /*
   * ========================================================
   * 현재 WARNING
   * ========================================================
   */
  if (state_data.current_state == VIBRATION_STATE_WARNING) {
    /*
     * 더 큰 진동이 발생하면 DANGER.
     */
    if (vibration_value >= state_config.danger_threshold) {
      return VIBRATION_STATE_DANGER;
    }

    /*
     * NORMAL 복귀 조건.
     *
     * 단순히 warning_threshold 아래가 아니라
     *
     * warning_threshold - hysteresis
     *
     * 까지 충분히 내려와야 한다.
     */
    if (vibration_value <=
        (state_config.warning_threshold - state_config.hysteresis)) {
      return VIBRATION_STATE_NORMAL;
    }

    return VIBRATION_STATE_WARNING;
  }

  /*
   * ========================================================
   * 현재 DANGER
   * ========================================================
   */
  if (state_data.current_state == VIBRATION_STATE_DANGER) {
    /*
     * DANGER에서 벗어나려면
     *
     * danger_threshold - hysteresis
     *
     * 아래로 충분히 내려와야 한다.
     *
     * 안전하게 DANGER → WARNING 순서로 복귀한다.
     */
    if (vibration_value <=
        (state_config.danger_threshold - state_config.hysteresis)) {
      return VIBRATION_STATE_WARNING;
    }

    return VIBRATION_STATE_DANGER;
  }

  /*
   * 예상하지 못한 상태라면
   * 안전하게 NORMAL 후보 반환.
   */
  return VIBRATION_STATE_NORMAL;
}

/*
 * ============================================================
 * 초기화
 * ============================================================
 */
void vibration_state_init(void) {
  /*
   * 설정값 초기화.
   */
  memset(&state_config, 0, sizeof(state_config));

  /*
   * 상태 데이터 초기화.
   */
  memset(&state_data, 0, sizeof(state_data));

  /*
   * 시스템 시작 상태는 NORMAL.
   */
  state_data.current_state = VIBRATION_STATE_NORMAL;

  state_data.candidate_state = VIBRATION_STATE_NORMAL;
}

/*
 * ============================================================
 * 상태 판정 설정
 * ============================================================
 */
bool vibration_state_set_config(const vibration_state_config_t *config) {
  /*
   * 잘못된 Pointer.
   */
  if (config == 0) {
    return false;
  }

  /*
   * WARNING 기준은
   * 0보다 커야 한다.
   */
  if (config->warning_threshold <= 0.0f) {
    return false;
  }

  /*
   * DANGER 기준은
   * WARNING보다 커야 한다.
   */
  if (config->danger_threshold <= config->warning_threshold) {
    return false;
  }

  /*
   * Hysteresis는 음수가 될 수 없다.
   *
   * 또한 WARNING Threshold보다 크면
   * NORMAL 복귀 기준이 음수가 될 수 있으므로
   * 허용하지 않는다.
   */
  if ((config->hysteresis < 0.0f) ||
      (config->hysteresis >= config->warning_threshold)) {
    return false;
  }

  /*
   * 최소 1회 이상 연속 판정 필요.
   */
  if (config->persistence_count == 0U) {
    return false;
  }

  /*
   * 유효한 설정값 저장.
   */
  state_config = *config;

  /*
   * 상태 판정 활성화.
   */
  state_data.configured = true;

  /*
   * 기존 후보 판정 초기화.
   */
  state_data.candidate_state = state_data.current_state;

  state_data.candidate_count = 0U;

  state_data.state_changed = false;

  return true;
}

/*
 * ============================================================
 * 상태 판정 Update
 * ============================================================
 */
bool vibration_state_update(float vibration_value) {
  vibration_state_t next_state;

  /*
   * 이번 Update에서는
   * 아직 상태 변화 없음.
   */
  state_data.state_changed = false;

  /*
   * 최근 입력값 저장.
   *
   * Live Watch에서 확인할 수 있다.
   */
  state_data.vibration_value = vibration_value;

  /*
   * Update 횟수 증가.
   */
  state_data.update_count++;

  /*
   * Threshold 설정 전에는
   * 실제 상태 판정을 하지 않는다.
   */
  if (!state_data.configured) {
    return false;
  }

  /*
   * RMS는 음수가 될 수 없다.
   */
  if (vibration_value < 0.0f) {
    return false;
  }

  /*
   * 현재 진동값으로
   * 다음 후보 상태를 판단한다.
   */
  next_state = vibration_state_determine(vibration_value);

  /*
   * ========================================================
   * 현재 상태와 동일
   * ========================================================
   *
   * 이상 상태가 연속되지 않았으므로
   * 후보 Counter를 초기화한다.
   */
  if (next_state == state_data.current_state) {
    state_data.candidate_state = state_data.current_state;

    state_data.candidate_count = 0U;

    return false;
  }

  /*
   * ========================================================
   * 새로운 후보 상태 발견
   * ========================================================
   */
  if (next_state != state_data.candidate_state) {
    state_data.candidate_state = next_state;

    state_data.candidate_count = 1U;
  }

  /*
   * ========================================================
   * 같은 후보 상태가 연속 발생
   * ========================================================
   */
  else {
    if (state_data.candidate_count < UINT8_MAX) {
      state_data.candidate_count++;
    }
  }

  /*
   * 아직 필요한 연속 횟수에
   * 도달하지 않았다.
   */
  if (state_data.candidate_count < state_config.persistence_count) {
    return false;
  }

  /*
   * ========================================================
   * 실제 상태 변경
   * ========================================================
   */
  state_data.current_state = state_data.candidate_state;

  state_data.candidate_count = 0U;

  state_data.change_count++;

  state_data.state_changed = true;

  return true;
}

/*
 * ============================================================
 * 전체 상태 데이터 반환
 * ============================================================
 */
void vibration_state_get_data(vibration_state_data_t *data) {
  if (data == 0) {
    return;
  }

  *data = state_data;
}

/*
 * ============================================================
 * 현재 상태 반환
 * ============================================================
 */
vibration_state_t vibration_state_get_state(void) {
  return state_data.current_state;
}

/*
 * ============================================================
 * 상태 문자열 반환
 * ============================================================
 */
const char *vibration_state_get_name(vibration_state_t state) {
  if (state == VIBRATION_STATE_NORMAL) {
    return "NORMAL";
  }

  if (state == VIBRATION_STATE_WARNING) {
    return "WARNING";
  }

  if (state == VIBRATION_STATE_DANGER) {
    return "DANGER";
  }

  return "UNKNOWN";
}
