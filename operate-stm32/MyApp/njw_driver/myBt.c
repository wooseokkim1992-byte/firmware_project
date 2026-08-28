#include "myBt.h"
#include "usart.h"
#include "stdio.h"
#include "string.h"

#define BT_UART         huart1

#define BT_NAME         "=fan"
#define BT_PASSWORD     "1234"

/*
 * INQ에서 확인된 Slave 주소
 * +INQ:98DA:60:0CDE01,...
 *
 * AT 명령에서는 ':' 대신 ',' 사용
 */
#define BT_SLAVE_ADDR   "98DA,60,0CDE01"

#define BT_RX_SIZE      256


/* =========================================================
 * 내부 함수
 * ========================================================= */

/*
 * 이전 AT 명령의 응답이 UART RX에 남아있으면 제거
 */
static void bt_ClearRx(void)
{
    uint8_t dummy;

    while (HAL_UART_Receive(&BT_UART,
                            &dummy,
                            1,
                            10) == HAL_OK)
    {
    }
}


/*
 * AT 명령 공통 송수신 함수
 *
 * first_timeout:
 *   첫 번째 응답 바이트가 올 때까지 기다리는 시간
 *
 * idle_timeout:
 *   응답이 들어오기 시작한 뒤,
 *   이 시간 동안 추가 바이트가 없으면 응답 종료로 판단
 *
 * return:
 *   실제 수신한 바이트 수
 */
static uint16_t bt_SendAT(const char *cmd,
                          uint8_t *rx,
                          uint16_t rx_size,
                          uint32_t first_timeout,
                          uint32_t idle_timeout)
{
    uint16_t index = 0;

    bt_ClearRx();

    memset(rx, 0, rx_size);

    HAL_StatusTypeDef tx_status =
        HAL_UART_Transmit(&BT_UART,
                          (uint8_t *)cmd,
                          strlen(cmd),
                          1000);

    printf("\r\n============================\r\n");
    printf("CMD : %s", cmd);
    printf("TX status : %d\r\n", tx_status);

    if (tx_status != HAL_OK)
    {
        printf("TX FAIL\r\n");
        return 0;
    }

    uint32_t start_tick = HAL_GetTick();
    uint32_t last_rx_tick = start_tick;

    uint8_t received = 0;

    while (1)
    {
        if (index >= rx_size - 1)
        {
            break;
        }

        HAL_StatusTypeDef status =
            HAL_UART_Receive(&BT_UART,
                             &rx[index],
                             1,
                             20);

        if (status == HAL_OK)
        {
            index++;
            received = 1;
            last_rx_tick = HAL_GetTick();
        }

        /*
         * 아직 아무 데이터도 안 들어온 상태
         */
        if (received == 0)
        {
            if ((HAL_GetTick() - start_tick) >= first_timeout)
            {
                break;
            }
        }

        /*
         * 데이터가 한 번이라도 들어온 상태
         */
        else
        {
            if ((HAL_GetTick() - last_rx_tick) >= idle_timeout)
            {
                break;
            }
        }
    }

    rx[index] = '\0';

    printf("RX count : %u\r\n", index);

    if (index > 0)
    {
        printf("RX : %s\r\n", rx);
    }
    else
    {
        printf("RX : <NO RESPONSE>\r\n");
    }

    return index;
}


/* =========================================================
 * AT 기본 확인
 * ========================================================= */

void bt_AtIsOk(void)
{
    uint8_t rx[32];

    bt_SendAT("AT+BIND?\r\n",
              rx,
              sizeof(rx),
              2000,
              200);
}


/* =========================================================
 * 이름 설정
 * ========================================================= */

void bt_SetName(void)
{
    uint8_t rx[32];

    bt_SendAT("AT+NAME=" BT_NAME "\r\n",
              rx,
              sizeof(rx),
              2000,
              200);
}


/* =========================================================
 * PIN 설정
 * ========================================================= */

void bt_SetPassword(void)
{
    uint8_t rx[32];

    bt_SendAT("AT+PSWD=" BT_PASSWORD "\r\n",
              rx,
              sizeof(rx),
              2000,
              200);
}


/* =========================================================
 * Master 설정
 * ROLE=1
 * ========================================================= */

void bt_SetRole(void)
{
    uint8_t rx[32];

    bt_SendAT("AT+ROLE=1\r\n",
              rx,
              sizeof(rx),
              2000,
              200);
}


/* =========================================================
 * 검색용 CMODE
 *
 * CMODE=1
 * 주변 장치 검색/연결 허용
 * ========================================================= */

void bt_SetSearchMode(void)
{
    uint8_t rx[32];

    bt_SendAT("AT+CMODE=1\r\n",
              rx,
              sizeof(rx),
              2000,
              200);
}


/* =========================================================
 * Inquiry 설정
 * ========================================================= */

void bt_SearchType(void)
{
    uint8_t rx[32];

    bt_SendAT("AT+INQM=0,5,9\r\n",
              rx,
              sizeof(rx),
              2000,
              200);
}


/* =========================================================
 * SPP 초기화
 *
 * 일부 HC-05/클론에서는 ERROR가 나올 수 있음.
 * INQ가 정상 동작한다면 반드시 치명적인 오류는 아님.
 * ========================================================= */

void bt_CanConnect(void)
{
    uint8_t rx[64];

    bt_SendAT("AT+INIT\r\n",
              rx,
              sizeof(rx),
              2000,
              300);
}


/* =========================================================
 * 현재 HC-05 상태 확인
 * ========================================================= */

void bt_GetState(void)
{
    uint8_t rx[64];

    bt_SendAT("AT+STATE?\r\n",
              rx,
              sizeof(rx),
              2000,
              300);
}


/* =========================================================
 * Slave 검색
 * ========================================================= */

static void bt_IsBind(void)
{
    uint8_t rx[64];

    bt_SendAT("AT+STATE?\r\n",
              rx,
              sizeof(rx),
              2000,
              300);
}


void bt_SearchSlave(void)
{
    uint8_t rx[BT_RX_SIZE] = {0};

    bt_ClearRx();

    HAL_UART_Transmit(&BT_UART,
                      (uint8_t *)"AT+INQ\r\n",
                      strlen("AT+INQ\r\n"),
                      1000);

    printf("\r\n============================\r\n");
    printf("CMD : AT+INQ\r\n");

    uint16_t index = 0;
    uint32_t start_tick = HAL_GetTick();

    /*
     * Inquiry는 시간이 오래 걸릴 수 있으므로
     * 최대 15초 수집
     */
    while ((HAL_GetTick() - start_tick) < 15000)
    {
        if (index >= sizeof(rx) - 1)
        {
            break;
        }

        if (HAL_UART_Receive(&BT_UART,
                             &rx[index],
                             1,
                             50) == HAL_OK)
        {
            index++;
        }
    }

    rx[index] = '\0';

    printf("INQ count : %u\r\n", index);
    printf("INQ RESULT:\r\n");
    printf("%s\r\n", rx);
    printf("INQ END\r\n");
}


/* =========================================================
 * Inquiry 강제 종료
 * ========================================================= */

void bt_StopSearch(void)
{
    uint8_t rx[32];

    bt_SendAT("AT+INQC\r\n",
              rx,
              sizeof(rx),
              2000,
              300);
}


/* =========================================================
 * 대상 이름 조회
 *
 * 이미 우리가 RNAME으로 :AT#20 / =fan 등을 확인했으므로
 * 주소 확인용
 * ========================================================= */

void bt_GetSlaveName(void)
{
    uint8_t rx[64];
    char command[64];

    snprintf(command,
             sizeof(command),
             "AT+RNAME?%s\r\n",
             BT_SLAVE_ADDR);

    bt_SendAT(command,
              rx,
              sizeof(rx),
              3000,
              300);
}


/* =========================================================
 * PAIR
 * ========================================================= */

void bt_Pair(void)
{
    uint8_t rx[64] = {0};
    char command[64] = {0};

    snprintf(command,
             sizeof(command),
             "AT+PAIR=%s,20\r\n",
             BT_SLAVE_ADDR);

    uint16_t rx_count =
        bt_SendAT(command,
                  rx,
                  sizeof(rx),
                  25000,
                  500);

    printf("\r\nPAIR RESULT\r\n");
    printf("PAIR RX count : %u\r\n", rx_count);

    if (rx_count > 0)
    {
        printf("PAIR RX : %s\r\n", rx);
    }
    else
    {
        printf("PAIR RX : <NO RESPONSE>\r\n");
    }
}


/* =========================================================
 * BIND
 *
 * 특정 Slave 주소를 HC-05에 등록
 * ========================================================= */

void bt_Bind(void)
{
    uint8_t rx[64];
    char command[64];

    snprintf(command,
             sizeof(command),
             "AT+BIND=%s\r\n",
             BT_SLAVE_ADDR);

    bt_SendAT(command,
              rx,
              sizeof(rx),
              3000,
              300);
}


/* =========================================================
 * 특정 주소만 연결하도록 변경
 *
 * CMODE=0
 * ========================================================= */

void bt_SetOnlyTarget(void)
{
    uint8_t rx[32];

    bt_SendAT("AT+CMODE=0\r\n",
              rx,
              sizeof(rx),
              2000,
              200);
}


/* =========================================================
 * LINK
 *
 * 실제 연결 시도
 * ========================================================= */

void bt_Link(void)
{
    uint8_t rx[64];
    char command[64];

    snprintf(command,
             sizeof(command),
             "AT+LINK=%s\r\n",
             BT_SLAVE_ADDR);

    bt_SendAT(command,
              rx,
              sizeof(rx),
              10000,
              500);
}


/* =========================================================
 * HC-05 전체 초기 세팅
 * ========================================================= */

void bt_Reset(void)
{
    printf("\r\n");
    printf("========================================\r\n");
    printf("HC-05 MASTER SETUP START\r\n");
    printf("========================================\r\n");

    /*
     * 1. AT 통신 확인
     */
    bt_AtIsOk();
    bt_IsBind();
    bt_Bind();
    /*
     * 2. HC-05 기본 설정
     */
    bt_SetName();
    bt_SetPassword();
    bt_SetRole();

    /*
     * 3. 주변 장치 검색 가능 모드
     */
    bt_SetSearchMode();

    /*
     * 4. Inquiry 설정
     */
    bt_SearchType();

    /*
     * 5. SPP INIT
     *
     * 현재 네 모듈에서 ERROR:(0)이 나왔던 부분.
     * 일단 응답 확인용으로 유지.
     */
    // bt_CanConnect();

    /*
     * 6. 현재 상태 확인
     */
    // bt_GetState();

    /*
     * 7. 상대 장치 이름 확인
     *
     * 이미 주소가 확인된 상황이라
     * RNAME으로 접근 가능한지 재확인.
     */
    // bt_GetSlaveName();

    // 주소를 다시 검색하고 싶으면 아래 사용.
    // bt_SearchSlave();
    // bt_StopSearch();


    /*
     * 8. PAIR
     */
    bt_Pair();

    /*
     * ===== 지금은 여기까지만 먼저 테스트 =====
     *
     * PAIR에서 OK가 확인된 후 아래 주석 해제.
     */

    
    bt_Bind();

    bt_SetOnlyTarget();

    bt_Link();

    bt_GetState();
    
    bt_test_2();

    printf("\r\n");
    printf("========================================\r\n");
    printf("HC-05 MASTER SETUP END\r\n");
    printf("========================================\r\n");
}

void bt_test_1(void){
    uint8_t tx[] = "HELLO\r\n";

    HAL_UART_Transmit(&huart1,
                  tx,
                  sizeof(tx) - 1,
                  1000);
}

void bt_test_2(void){
    uint8_t rx[16] = {0};

    HAL_UART_Receive(&huart1,
                     rx,
                     7,
                     3000);

    printf("%s\r\n", rx);
}