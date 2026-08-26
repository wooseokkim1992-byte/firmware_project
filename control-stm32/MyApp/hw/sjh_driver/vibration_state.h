#ifndef VIBRATION_STATE_H
#define VIBRATION_STATE_H

#include <stdint.h>
#include <stdbool.h>


/*
 * ============================================================
 * Vibration State Decision
 * ============================================================
 *
 * 담당 기능 : 중분류 1.3 진동 상태 판정
 *
 * vibration.c에서 계산한 대표 진동값(RMS)을 이용하여
 *
 * NORMAL
 * WARNING
 * DANGER
 *
 * 상태를 판정한다.
 *
 *
 * 주요 기능
 *
 * 1. NORMAL / WARNING / DANGER 판정
 * 2. 임계값 관리
 * 3. 히스테리시스 적용
 * 4. 지속시간 검사
 * 5. 상태 변경 이벤트 생성
 *
 *
 * ※ 실제 WARNING / DANGER 임계값은
 *    임의로 코드에 고정하지 않는다.
 *
 * 실제 회전체 측정 후
 * vibration_state_set_config()를 통해 설정한다.
 * ============================================================
 */


/*
 * ============================================================
 * 진동 상태
 * ============================================================
 */
typedef enum
{
    VIBRATION_STATE_NORMAL = 0,
    VIBRATION_STATE_WARNING,
    VIBRATION_STATE_DANGER

} vibration_state_t;


/*
 * ============================================================
 * 상태 판정 설정값
 * ============================================================
 *
 * 실제 회전체 측정 결과를 바탕으로
 * 이후 설정할 값들이다.
 */
typedef struct
{
    /*
     * WARNING 진입 기준
     *
     * 단위 : g
     */
    float warning_threshold;


    /*
     * DANGER 진입 기준
     *
     * 단위 : g
     */
    float danger_threshold;


    /*
     * 상태 흔들림 방지를 위한 Hysteresis
     *
     * 예:
     *
     * warning_threshold = 0.05g
     * hysteresis        = 0.01g
     *
     * WARNING에서 NORMAL로 복귀하려면
     *
     * 0.04g 이하
     *
     * 까지 내려와야 한다.
     */
    float hysteresis;


    /*
     * 상태가 실제 변경되기 위해 필요한
     * 연속 판정 횟수.
     *
     * 예:
     *
     * persistence_count = 3
     *
     * 이라면 같은 이상 상태가
     * 3번 연속 확인되어야 상태를 변경한다.
     */
    uint8_t persistence_count;

} vibration_state_config_t;


/*
 * ============================================================
 * 상태 판정 결과
 * ============================================================
 */
typedef struct
{
    /*
     * 현재 확정된 상태.
     */
    vibration_state_t current_state;


    /*
     * 현재 연속으로 관찰 중인 후보 상태.
     *
     * 예:
     *
     * 현재 NORMAL인데
     * WARNING 조건이 처음 감지되었다면
     *
     * candidate_state = WARNING
     */
    vibration_state_t candidate_state;


    /*
     * 가장 최근 상태 판정에 사용한 진동값.
     *
     * 현재는 vibration_rms가 전달될 예정이다.
     */
    float vibration_value;


    /*
     * 후보 상태가 연속으로 감지된 횟수.
     */
    uint8_t candidate_count;


    /*
     * 상태 판정 Update 실행 횟수.
     *
     * Live Watch 확인용.
     */
    uint32_t update_count;


    /*
     * 실제 상태가 변경된 횟수.
     */
    uint32_t change_count;


    /*
     * 임계값 설정 완료 여부.
     *
     * false인 경우
     * 실제 상태 판정을 수행하지 않는다.
     */
    bool configured;


    /*
     * 가장 최근 Update에서
     * 상태 변경이 발생했는지 표시.
     */
    bool state_changed;

} vibration_state_data_t;


/*
 * ============================================================
 * 초기화
 * ============================================================
 */
void vibration_state_init(void);


/*
 * ============================================================
 * 상태 판정 설정
 * ============================================================
 *
 * 정상 동작 조건:
 *
 * warning_threshold > 0
 *
 * danger_threshold > warning_threshold
 *
 * hysteresis >= 0
 *
 * persistence_count > 0
 *
 * 성공 : true
 * 실패 : false
 */
bool vibration_state_set_config(
    const vibration_state_config_t *config
);


/*
 * ============================================================
 * 상태 판정 Update
 * ============================================================
 *
 * 입력:
 * vibration.c에서 만들어진 대표 진동값
 *
 * 반환:
 *
 * true
 * → 이번 Update에서 실제 상태가 변경됨
 *
 * false
 * → 상태 유지
 */
bool vibration_state_update(float vibration_value);


/*
 * 현재 상태 판정 결과 전체 반환.
 */
void vibration_state_get_data(
    vibration_state_data_t *data
);


/*
 * 현재 확정된 상태만 반환.
 */
vibration_state_t vibration_state_get_state(void);


/*
 * 상태를 문자열로 반환.
 *
 * UART Debug 등에 사용할 수 있다.
 *
 * NORMAL
 * WARNING
 * DANGER
 */
const char *vibration_state_get_name(
    vibration_state_t state
);


#endif