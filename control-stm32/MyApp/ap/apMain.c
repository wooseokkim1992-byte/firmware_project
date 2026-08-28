#include "apMain.h"
#include "myI2c.h"

#include "hw/sjh_driver/mpu6050.h"
#include "hw/sjh_driver/vibration.h"
#include "hw/sjh_driver/vibration_state.h"

#include "hw/kws_driver/kws_adc.h"
#include "hw/kws_driver/sound_level.h"
#include "hw/kws_driver/sound_offset.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

// sound 측정에 필요한 데이터 목록
static int16_t sound_centered_samples[ADC_WINDOW_SIZE];
static uint16_t sound_dc_offset = 0U;
static sound_level_data_t sound_level_result = {0};

/* Sound NORMAL baseline / Z-score detector test parameters.
 * Adjust these values after collecting data from the assembled rotor. */
static sound_level_detector_config_t sound_level_detector_config = {
    .rms_warning_z = 1.5f,
    .peak_warning_z = 4.8f,
    .rms_normal_z = 1.4f,
    .peak_normal_z = 4.0f,
    .stddev_epsilon = 1.0f,
    .warning_persistence = 3U,
    .normal_persistence = 5U};

/* Live Watch variables for the latest sound detector result. */
static sound_level_detector_data_t sound_level_detector_data = {0};
static sound_level_state_t sound_current_state = SOUND_LEVEL_STATE_NORMAL;
static bool sound_level_state_changed = false;
static bool sound_level_detector_configured = false;
/*
 * ============================================================
 * Control STM32 - Application Main
 * ============================================================
 *
 * 현재 구현
 *
 * 1.1 MPU6050 센서 제어
 * 1.2 진동 데이터 처리
 *
 * 데이터 흐름
 *
 * MPU6050
 *     ↓
 * accel_x / accel_y / accel_z
 *     ↓
 * vibration_update()
 *     ↓
 * Mean / RMS / Peak
 *     ↓
 * vibration_value
 *
 *
 * 다음 단계
 *
 * 1.3 vibration_state.c
 *
 * vibration_value를 이용하여
 *
 * NORMAL
 * WARNING
 * DANGER
 *
 * 상태를 판단한다.
 * ============================================================
 */

/*
 * MPU6050 데이터
 */
static mpu6050_data_t mpu_data = {0};

/*
 * 진동 처리 결과
 *
 * Live Watch에서
 *
 * vibration_data.vibration_rms
 * vibration_data.vibration_peak
 *
 * 등을 확인할 수 있다.
 */
static vibration_data_t vibration_data = {0};

/*
 * 진동 상태 판정 결과
 *
 * Live Watch에서
 *
 * vibration_state_data.current_state
 * vibration_state_data.candidate_state
 * vibration_state_data.candidate_count
 * vibration_state_data.change_count
 *
 * 등을 확인할 수 있다.
 */

/*
 * ============================================================
 * 진동 상태 판정 임시 테스트 설정
 * ============================================================
 *
 * 아래 Threshold 값은 상태 판정 로직 검증용 임시값이다.
 *
 * 실제 FAN / 발전기 구조의 최종 Threshold가 아니다.
 *
 * 실제 회전체 조립 후
 * NORMAL / WARNING / DANGER 상태의 RMS 데이터를
 * 반복 측정한 뒤 반드시 다시 설정한다.
 *
 * 현재 테스트값:
 * WARNING : 0.005 g
 * DANGER  : 0.010 g
 *
 * 손으로 센서를 조금만 움직여도 DANGER가 될 수 있음.
 * ============================================================
 */
static vibration_state_config_t vibration_state_config = {
    .warning_threshold = 0.055f,
    .danger_threshold = 0.065f,
    .hysteresis = 0.001f,
    .persistence_count = 3U};

static vibration_state_data_t vibration_state_data = {0};

/*
 * 새로운 64 Sample 진동 Window가
 * 만들어졌는지 확인.
 */
static bool vibration_updated = false;

/*
 * 가장 최근 상태 변경 여부.
 */
static bool vibration_state_changed = false;

/*
 * MPU6050 초기화 상태
 */
static bool mpu_init_status = false;

/*
 * MPU6050 Read 상태
 */
static bool mpu_read_status = false;

/*
 * MPU6050 Read 성공 횟수
 */
static uint32_t mpu_read_count = 0U;

/*
 * MPU6050 Read Error 횟수
 */
static uint32_t mpu_error_count = 0U;

/*
 * MPU6050 연속 실패 횟수
 */
static uint8_t mpu_fail_count = 0U;

/*
 * MPU6050 Driver 상태
 */
static mpu6050_status_t mpu_driver_status = MPU6050_STATUS_NOT_INITIALIZED;

/*
 * MPU6050 Sampling
 *
 * 125Hz
 *
 * = 약 8ms
 */
#define MPU_READ_PERIOD_MS 8U

/*
 * UART Debug 출력 주기
 */
#define DEBUG_PRINT_PERIOD_MS 1000U

static void sound_process_window(const uint16_t *raw_samples) {
  if (!sound_offset_remove_window(raw_samples, sound_centered_samples,
                                  ADC_WINDOW_SIZE, &sound_dc_offset)) {
    return;
  }

  if (!sound_level_calculate(sound_centered_samples, ADC_WINDOW_SIZE,
                             &sound_level_result)) {
    return;
  }

  sound_level_state_changed = sound_level_detector_update(&sound_level_result);
  sound_level_detector_get_data(&sound_level_detector_data);

  printf(">sound_offset:%u\r\n", sound_dc_offset);
  printf(">sound_mean_abs:%.2f\r\n", sound_level_result.mean_absolute);
  printf(">sound_rms:%.2f\r\n", sound_level_result.rms);
  printf(">sound_peak:%u\r\n", sound_level_result.peak);
  printf(">sound_rms_z:%.2f\r\n", sound_level_detector_data.rms_z_score);
  printf(">sound_peak_z:%.2f\r\n", sound_level_detector_data.peak_z_score);

  if (sound_level_state_changed) {
    sound_current_state = sound_level_detector_data.current_state;
    printf("[SOUND STATE] %s\r\n",
           sound_level_state_get_name(sound_current_state));
  }
}

/*
 * ============================================================
 * Application Initialization
 * ============================================================
 */
void apInit(void) {
  /*
   * UART 초기화
   */
  uartInit();

  /*
   * 현재 I2C Device 확인.
   *
   * 프로젝트 초기 Debug 용도로 유지.
   */
  i2cScan();

  /*
   * MPU6050 초기화
   */
  mpu_init_status = mpu6050_init();

  /*
   * MPU6050 상태 확인
   */
  mpu_driver_status = mpu6050_get_status();

  /*
   * 진동 처리 모듈 초기화
   */
  vibration_init();

  /*
   * 진동 상태 판정 모듈 초기화
   */
  vibration_state_init();

  /*
   * 진동 상태 판정 Test 설정 적용
   */
  vibration_state_set_config(&vibration_state_config);

  vibration_state_get_data(&vibration_state_data);

  /* Sound detector starts in NORMAL and first learns 20 baseline windows. */
  sound_level_detector_init();
  sound_level_detector_configured =
      sound_level_detector_set_config(&sound_level_detector_config);
  sound_level_detector_get_data(&sound_level_detector_data);
  sound_current_state = sound_level_detector_data.current_state;

  /*
   * 초기 상태를 Application 변수에 복사
   */
  vibration_state_get_data(&vibration_state_data);
  /*
   * MPU6050 초기화 결과 출력
   */
  if (mpu_init_status) {
    printf("\r\n");

    printf("============================\r\n");

    printf(" Control STM32 Start\r\n");

    printf("============================\r\n");

    printf("[MPU6050] Init Success\r\n");

    printf("[MPU6050] WHO_AM_I : 0x%02X\r\n", mpu6050_get_chip_id());

    printf("[MPU6050] Sample Rate : %lu Hz\r\n",
           (unsigned long)MPU6050_SAMPLE_RATE_HZ);

    printf("[VIBRATION] Window : %u Samples\r\n", VIBRATION_WINDOW_SIZE);
  } else {
    printf("\r\n");

    // printf("[MPU6050] Init Failed\r\n");

    // printf("[MPU6050] Status : %d\r\n", (int)mpu_driver_status);
  }
  if (!sound_level_detector_configured) {
    printf("[SOUND] Invalid detector configuration\r\n");
  }

  if (adc_init()) {
    printf("Start Detecting Sound Signals\r\n");
  } else {
    printf("Failed to start Sound ADC\r\n");
  }
}

/*
 * ============================================================
 * Application Main
 * ============================================================
 */
void apMain(void) {
  /*
   * MPU6050 마지막 읽기 시간
   */
  uint32_t tick_mpu = 0U;

  /*
   * Debug 마지막 출력 시간
   */
  uint32_t tick_debug = 0U;

  /*
   * MPU6050 재초기화 시간
   */
  uint32_t tick_reinit = 0U;

  /*
   * 현재 System Tick
   */
  uint32_t current_tick = 0U;

  while (1) {
    /*
     * 현재 시간
     *
     * 단위 : ms
     */
    current_tick = HAL_GetTick();
    /*
     * DMA 콜백이 100ms마다 준비한 구간을 즉시 처리한다.
     */
    if (adc_half_ready) {
      adc_half_ready = false;
      sound_process_window(&adc_dma_buffer[0]);
    }

    if (adc_full_ready) {
      adc_full_ready = false;
      sound_process_window(&adc_dma_buffer[ADC_WINDOW_SIZE]);
    }

    /*
     * ====================================================
     * MPU6050 Sampling
     * ====================================================
     *
     * 약 8ms마다 실행
     *
     * ≈ 125Hz
     */
    if ((current_tick - tick_mpu) >= MPU_READ_PERIOD_MS) {
      tick_mpu = current_tick;

      /*
       * MPU6050 초기화 성공 상태에서만 Read
       */
      if (mpu_init_status) {
        mpu_read_status = mpu6050_read(&mpu_data);

        /*
         * MPU6050 Read 성공
         */
        if (mpu_read_status) {
          mpu_read_count++;

          mpu_fail_count = 0U;

          /*
           * ==========================================
           * 1.2 진동 데이터 처리
           * ==========================================
           *
           * MPU6050에서 읽은
           *
           * accel_x
           * accel_y
           * accel_z
           *
           * 를 vibration 모듈에 전달한다.
           *
           * 반환값 true는
           * 64 Sample Window 결과가 새로 만들어졌다는 뜻.
           */
          /*
           * 진동 데이터 처리.
           *
           * true:
           * 새로운 64 Sample Window 결과가 만들어짐
           *
           * false:
           * 아직 Window 수집 중
           */
          vibration_updated = vibration_update(
              mpu_data.accel_x, mpu_data.accel_y, mpu_data.accel_z);

          /*
           * 최신 진동 처리 결과 복사
           */
          vibration_get_data(&vibration_data);

          /*
           * ========================================================
           * 1.3 진동 상태 판정
           * ========================================================
           *
           * 새로운 RMS 결과가 만들어졌을 때만
           * 상태 판정을 한 번 실행한다.
           */
          if (vibration_updated) {
            vibration_state_changed =
                vibration_state_update(vibration_data.vibration_value);

            vibration_state_get_data(&vibration_state_data);
          }
        }

        /*
         * MPU6050 Read 실패
         */
        else {
          mpu_error_count++;

          mpu_fail_count++;

          mpu_driver_status = mpu6050_get_status();

          /*
           * 3회 연속 실패
           */
          if (mpu_fail_count >= 3U) {
            mpu_init_status = false;

            mpu_fail_count = 0U;

            tick_reinit = current_tick;
          }
        }
      }
    }

    /*
     * ====================================================
     * MPU6050 재초기화
     * ====================================================
     */
    if (!mpu_init_status) {
      if ((current_tick - tick_reinit) >= 1000U) {
        tick_reinit = current_tick;

        mpu_init_status = mpu6050_reinit();

        mpu_driver_status = mpu6050_get_status();

        if (mpu_init_status) {
          mpu_read_status = false;

          mpu_fail_count = 0U;

          /*
           * 센서가 재초기화됐으므로
           * 기존 진동 Filter 기준값도 다시 초기화한다.
           */
          vibration_init();
        }
      }
    }

    /*
     * ====================================================
     * Debug Print
     * ====================================================
     *
     * Sensor Sampling은 8ms이지만
     * UART Print는 1초마다 실행한다.
     */
    if ((current_tick - tick_debug) >= DEBUG_PRINT_PERIOD_MS) {
      tick_debug = current_tick;

      if (mpu_read_status) {
        /*
         * MPU6050 원본 가속도
         */
        printf(">acc_x:%.4f\r\n"
               ">acc_y:%.4f\r\n"
               ">acc_z:%.4f\r\n",
               mpu_data.accel_x, mpu_data.accel_y, mpu_data.accel_z);

        /*
         * 진동 처리 결과
         *
         * Teleplot에서도 바로 확인 가능.
         */
        printf(">vibration:%.5f\r\n"
               ">vibration_mean:%.5f\r\n"
               ">vibration_rms:%.5f\r\n"
               ">vibration_peak:%.5f\r\n",
               vibration_data.vibration_magnitude,
               vibration_data.vibration_mean, vibration_data.vibration_rms,
               vibration_data.vibration_peak);
        printf("[VIBRATION STATE] %s\r\n",
               vibration_state_get_name(vibration_state_data.current_state));
      }

      else if (!mpu_init_status) {
        // printf("[MPU6050] Error Status : %d\r\n", (int)mpu_driver_status);
      }
    }
  }
}
