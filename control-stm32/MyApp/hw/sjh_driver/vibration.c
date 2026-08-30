#include "vibration.h"

#include <math.h>
#include <string.h>


/*
 * ============================================================
 * 내부 데이터
 * ============================================================
 */


/*
 * 최종적으로 외부에 제공할 진동 결과.
 */
static vibration_data_t vibration_data = {0};


/*
 * 첫 번째 Sample인지 확인.
 *
 * 첫 Sample에서는 현재 가속도를
 * 중력 / Offset의 초기 기준값으로 사용한다.
 */
static bool first_sample = true;


/*
 * Noise Filter 내부 상태.
 *
 * 이전 Filter 결과를 저장해야
 * 다음 Sample에서 이어서 계산할 수 있다.
 */
static float filtered_x = 0.0f;
static float filtered_y = 0.0f;
static float filtered_z = 0.0f;


/*
 * ============================================================
 * Window 계산용 누적값
 * ============================================================
 */


/*
 * 진동 크기의 합.
 *
 * Mean 계산에 사용.
 */
static float vibration_sum = 0.0f;


/*
 * 진동 크기 제곱의 합.
 *
 * RMS 계산에 사용.
 */
static float vibration_square_sum = 0.0f;


/*
 * 현재 Window에서 가장 큰 진동값.
 */
static float vibration_peak = 0.0f;


/*
 * ============================================================
 * vibration_init
 * ============================================================
 */
void vibration_init(void)
{
    /*
     * 결과 구조체 전체 초기화.
     */
    memset(
        &vibration_data,
        0,
        sizeof(vibration_data)
    );


    /*
     * 첫 번째 Sample에서
     * 기준값을 잡기 위해 true로 설정.
     */
    first_sample = true;


    /*
     * Filter 상태 초기화.
     */
    filtered_x = 0.0f;
    filtered_y = 0.0f;
    filtered_z = 0.0f;


    /*
     * Window 누적값 초기화.
     */
    vibration_sum = 0.0f;

    vibration_square_sum = 0.0f;

    vibration_peak = 0.0f;
}


/*
 * ============================================================
 * vibration_update
 * ============================================================
 */
bool vibration_update(float accel_x,
                      float accel_y,
                      float accel_z)
{
    /*
     * 기준값을 제거한 진동 성분.
     */
    float vibration_raw_x = 0.0f;
    float vibration_raw_y = 0.0f;
    float vibration_raw_z = 0.0f;


    /*
     * 합성 진동 계산용.
     */
    float magnitude_square = 0.0f;


    /*
     * ========================================================
     * 첫 Sample 처리
     * ========================================================
     *
     * 시스템 시작 직후에는 아직
     * 중력 방향 / 설치각을 알 수 없다.
     *
     * 따라서 처음 읽은 가속도를
     * 최초 기준값으로 사용한다.
     *
     * 예:
     *
     * accel_x = +0.14
     * accel_y = +0.29
     * accel_z = -0.94
     *
     * 라면:
     *
     * base_x = +0.14
     * base_y = +0.29
     * base_z = -0.94
     *
     * 로 시작한다.
     */
    if (first_sample)
    {
        vibration_data.base_x = accel_x;

        vibration_data.base_y = accel_y;

        vibration_data.base_z = accel_z;


        filtered_x = 0.0f;
        filtered_y = 0.0f;
        filtered_z = 0.0f;


        first_sample = false;


        /*
         * 첫 번째 Sample은
         * 기준 설정용으로만 사용한다.
         */
        return false;
    }


    /*
     * ========================================================
     * 1.2.1 / 1.2.2
     * Offset + 중력 성분 기준값 추적
     * ========================================================
     *
     * Low Pass 방식으로
     * 천천히 변하는 기준값을 만든다.
     *
     * 새로운 Base =
     *
     * 이전 Base × 0.95
     *
     * +
     *
     * 현재 Accel × 0.05
     *
     *
     * 중력이나 센서 설치 각도처럼
     * 천천히 변하는 값은 Base가 따라가지만,
     *
     * 회전체 진동처럼 빠르게 변하는 값은
     * Base에 크게 반영되지 않는다.
     */
    vibration_data.base_x =
        (VIBRATION_BASE_ALPHA
        * vibration_data.base_x)
        +
        ((1.0f - VIBRATION_BASE_ALPHA)
        * accel_x);


    vibration_data.base_y =
        (VIBRATION_BASE_ALPHA
        * vibration_data.base_y)
        +
        ((1.0f - VIBRATION_BASE_ALPHA)
        * accel_y);


    vibration_data.base_z =
        (VIBRATION_BASE_ALPHA
        * vibration_data.base_z)
        +
        ((1.0f - VIBRATION_BASE_ALPHA)
        * accel_z);


    /*
     * ========================================================
     * 기준값 제거
     * ========================================================
     *
     * 실제 Sensor 값에서
     * 중력 + Offset 기준을 빼준다.
     *
     *
     * 예:
     *
     * 현재 X = 0.145g
     * 기준 X = 0.140g
     *
     * 진동 X = 0.005g
     *
     *
     * 따라서 기존처럼 약 1g의 중력을
     * 진동으로 잘못 계산하지 않는다.
     */
    vibration_raw_x =
        accel_x - vibration_data.base_x;


    vibration_raw_y =
        accel_y - vibration_data.base_y;


    vibration_raw_z =
        accel_z - vibration_data.base_z;


    /*
     * ========================================================
     * 1.2.3 Noise Filtering
     * ========================================================
     *
     * 1차 Low Pass Filter
     *
     * Filter 결과 =
     *
     * 이전 결과
     * +
     * alpha × (현재값 - 이전 결과)
     *
     *
     * alpha = 0.70
     *
     * 새로운 값을 비교적 많이 반영하면서
     * 순간적인 Noise를 조금 부드럽게 만든다.
     */
    filtered_x =
        filtered_x
        +
        VIBRATION_FILTER_ALPHA
        * (vibration_raw_x - filtered_x);


    filtered_y =
        filtered_y
        +
        VIBRATION_FILTER_ALPHA
        * (vibration_raw_y - filtered_y);


    filtered_z =
        filtered_z
        +
        VIBRATION_FILTER_ALPHA
        * (vibration_raw_z - filtered_z);


    /*
     * 외부에서 Live Watch로 확인할 수 있도록 저장.
     */
    vibration_data.vibration_x = filtered_x;

    vibration_data.vibration_y = filtered_y;

    vibration_data.vibration_z = filtered_z;


    /*
     * ========================================================
     * 1.2.4 X/Y/Z 합성 진동 크기
     * ========================================================
     *
     * 어느 방향으로 진동하더라도
     * 하나의 진동 크기로 표현하기 위해
     * 3축 Vector 크기를 계산한다.
     *
     *
     * magnitude =
     *
     * sqrt(
     *     X² +
     *     Y² +
     *     Z²
     * )
     */
    magnitude_square =
        (filtered_x * filtered_x)
        +
        (filtered_y * filtered_y)
        +
        (filtered_z * filtered_z);


    vibration_data.vibration_magnitude =
        sqrtf(magnitude_square);


    /*
     * ========================================================
     * Window 누적
     * ========================================================
     *
     * Mean 계산용.
     */
    vibration_sum +=
        vibration_data.vibration_magnitude;


    /*
     * RMS 계산용.
     *
     * magnitude² 값을 더해둔다.
     */
    vibration_square_sum +=
        magnitude_square;


    /*
     * ========================================================
     * Peak
     * ========================================================
     *
     * 현재 값이 기존 Peak보다 크면
     * 새로운 Peak로 저장한다.
     */
    if (vibration_data.vibration_magnitude
        > vibration_peak)
    {
        vibration_peak =
            vibration_data.vibration_magnitude;
    }


    /*
     * Sample 개수 증가.
     */
    vibration_data.sample_count++;


    /*
     * 아직 설정된 Window Sample이 모이지 않았다면
     * 결과 계산 없이 종료.
     */
    if (vibration_data.sample_count
        < VIBRATION_WINDOW_SIZE)
    {
        return false;
    }


    /*
     * ========================================================
     * 1.2.5 Mean
     * ========================================================
     *
     * Mean =
     *
     * 진동 크기의 총합 / Sample 개수
     */
    vibration_data.vibration_mean =
        vibration_sum
        / (float)VIBRATION_WINDOW_SIZE;


    /*
     * ========================================================
     * 1.2.5 RMS
     * ========================================================
     *
     * RMS =
     *
     * sqrt(
     *     진동 크기² 평균
     * )
     *
     *
     * RMS는 진동의 전체적인 크기를 나타내므로
     * 회전체 상태 모니터링에서 대표값으로 사용하기 좋다.
     */
    vibration_data.vibration_rms =
        sqrtf(
            vibration_square_sum
            / (float)VIBRATION_WINDOW_SIZE
        );


    /*
     * ========================================================
     * 1.2.5 Peak
     * ========================================================
     */
    vibration_data.vibration_peak =
        vibration_peak;


    /*
     * ========================================================
     * 1.2.7 대표 진동값
     * ========================================================
     *
     * 현재 프로젝트에서는
     * 대표 진동값으로 RMS를 사용한다.
     *
     * 이후 vibration_state.c에서는
     * 이 vibration_value를 받아
     *
     * NORMAL
     * WARNING
     * DANGER
     *
     * 판정에 사용할 수 있다.
     */
    vibration_data.vibration_value =
        vibration_data.vibration_rms;


    /*
     * ========================================================
     * 1.2.6 정상 상태와 차이 계산
     * ========================================================
     *
     * 정상 기준값이 실제 측정을 통해
     * 설정된 경우에만 계산한다.
     *
     * 임의의 정상값을 코드에 넣지 않는다.
     */
    if (vibration_data.normal_reference_set)
    {
        vibration_data.rms_difference =
            vibration_data.vibration_rms
            -
            vibration_data.normal_rms;
    }
    else
    {
        vibration_data.rms_difference = 0.0f;
    }


    /*
     * 새로운 결과가 만들어진 횟수 증가.
     */
    vibration_data.update_count++;


    /*
     * ========================================================
     * 다음 Window 준비
     * ========================================================
     */

    vibration_data.sample_count = 0U;


    vibration_sum = 0.0f;

    vibration_square_sum = 0.0f;

    vibration_peak = 0.0f;


    /*
     * 새로운 RMS / Peak 결과가 만들어졌음을 알린다.
     */
    return true;
}


/*
 * ============================================================
 * 현재 결과 전달
 * ============================================================
 */
void vibration_get_data(vibration_data_t *data)
{
    /*
     * 잘못된 Pointer이면 종료.
     */
    if (data == 0)
    {
        return;
    }


    /*
     * 현재 결과 전체 복사.
     */
    *data = vibration_data;
}


/*
 * ============================================================
 * 정상 RMS 기준값 설정
 * ============================================================
 */
void vibration_set_normal_rms(float normal_rms)
{
    /*
     * 음수 RMS는 존재할 수 없으므로
     * 0 미만 값은 무시한다.
     */
    if (normal_rms < 0.0f)
    {
        return;
    }


    vibration_data.normal_rms =
        normal_rms;


    vibration_data.normal_reference_set =
        true;
}


/*
 * ============================================================
 * 정상 RMS 기준값 삭제
 * ============================================================
 */
void vibration_clear_normal_rms(void)
{
    vibration_data.normal_rms = 0.0f;

    vibration_data.rms_difference = 0.0f;

    vibration_data.normal_reference_set = false;
}
