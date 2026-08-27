#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Telemetry packet (12 bytes)
 * [STX][PAYLOAD LENGTH][VIBRATION(float32 LE)][SOUND(float32 LE)]
 * [STATUS][CHECKSUM]
 *
 * CHECKSUM is the XOR of LENGTH through STATUS. Vibration and sound state
 * thresholds are intentionally left to the application until measured.
 * On Nucleo-F411RE, sound_level is supplied from PA0 / ADC1_IN0.
 */
#define MONITOR_PACKET_STX                  (0x02u)
#define MONITOR_COMMAND_HEARTBEAT            (0xA5u)
#define MONITOR_COMMAND_EMERGENCY_STOP       (0xEEu)
#define MONITOR_TELEMETRY_PAYLOAD_LENGTH     (9u)
#define MONITOR_TELEMETRY_PACKET_LENGTH      (12u)
#define MONITOR_RX_BUFFER_SIZE               (32u)
#define MONITOR_DISCONNECT_FAIL_COUNT        (10u)
#define MONITOR_EVENT_RETRY_INTERVAL_MS      (100u)

/* Provisional value; tune in the 30-60 second range after system testing. */
#define MONITOR_DEFAULT_LINK_TIMEOUT_MS      (45000u)

/* STATUS byte: state[1:0], sensor errors[5:4], relay state[6]. */
#define MONITOR_STATUS_STATE_MASK            (0x03u)
#define MONITOR_STATUS_VIBRATION_ERROR       (0x10u)
#define MONITOR_STATUS_SOUND_ERROR           (0x20u)
#define MONITOR_STATUS_RELAY_ON              (0x40u)
#define MONITOR_SENSOR_ERROR_MASK            \
    (MONITOR_STATUS_VIBRATION_ERROR | MONITOR_STATUS_SOUND_ERROR)

typedef enum {
    SYSTEM_STATE_NORMAL = 0u,
    SYSTEM_STATE_WARNING = 1u,
    SYSTEM_STATE_DANGER = 2u,
    SYSTEM_STATE_EMERGENCY_STOP = 3u
} SystemState_e;

/* MONITOR_TX_OK means that DMA accepted the buffer, not that it completed. */
typedef enum {
    MONITOR_TX_OK = 0,
    MONITOR_TX_FAILED,
    MONITOR_TX_BUFFER_OVERFLOW,
    MONITOR_TX_DISCONNECTED
} MonitorTxResult_e;

typedef struct {
    float vibration_level;
    float sound_level;
    SystemState_e current_state;
    uint8_t sensor_error_flags;
    bool relay_on;
} MonitorData_t;

typedef struct {
    uint16_t tx_fail_count;
    uint32_t tx_failure_total;
    uint32_t tx_buffer_overflow_total;
    uint32_t rx_buffer_overflow_total;
    uint32_t rx_invalid_packet_total;
    bool buffer_overflow;
    bool is_connected;
    bool tx_in_progress;
    MonitorTxResult_e last_tx_result;
} MonitorErrorState_t;

typedef MonitorTxResult_e (*MonitorTx_Function_t)(void *context,
                                                   const uint8_t *data,
                                                   uint16_t length);
typedef void (*MonitorEmergencyStop_Callback_t)(void *context);
typedef void (*MonitorLinkState_Callback_t)(void *context, bool connected);

typedef struct {
    uint32_t tx_interval_ms;
    uint32_t link_timeout_ms;
    uint32_t last_tx_attempt_time;
    uint32_t last_link_activity_time;
    uint8_t last_sent_status;
    uint8_t last_attempt_status;
    uint8_t in_flight_status;
    bool has_sent_packet;
    bool has_attempted_packet;
    bool has_link_activity;
    bool emergency_stop_latched;

    MonitorErrorState_t error_status;
    MonitorTx_Function_t Tx_Function;
    void *tx_context;
    MonitorEmergencyStop_Callback_t EmergencyStop_Callback;
    void *emergency_stop_context;
    MonitorLinkState_Callback_t LinkState_Callback;
    void *link_state_context;

    /* DMA reads this persistent buffer until MonitorTx_OnTxComplete(). */
    uint8_t tx_buffer[MONITOR_TELEMETRY_PACKET_LENGTH];
    uint8_t rx_buffer[MONITOR_RX_BUFFER_SIZE];
    uint16_t rx_length;
} MonitorTx_Handle_t;

void MonitorTx_Init(MonitorTx_Handle_t *htx,
                    uint32_t interval_ms,
                    MonitorTx_Function_t tx_func,
                    void *tx_context);

void MonitorTx_SetLinkTimeout(MonitorTx_Handle_t *htx,
                              uint32_t timeout_ms);

void MonitorTx_SetEmergencyStopCallback(
    MonitorTx_Handle_t *htx,
    MonitorEmergencyStop_Callback_t callback,
    void *context);

void MonitorTx_SetLinkStateCallback(MonitorTx_Handle_t *htx,
                                    MonitorLinkState_Callback_t callback,
                                    void *context);

void MonitorTx_ProcessPeriodic(MonitorTx_Handle_t *htx,
                               const MonitorData_t *data,
                               uint32_t current_time);

MonitorTxResult_e MonitorTx_SendEvent(MonitorTx_Handle_t *htx,
                                      const MonitorData_t *data);

/* Call from HAL_UART_TxCpltCallback for the configured monitoring UART. */
void MonitorTx_OnTxComplete(MonitorTx_Handle_t *htx);

/* Call from HAL_UART_ErrorCallback or when DMA start fails. */
void MonitorTx_OnTxError(MonitorTx_Handle_t *htx,
                         MonitorTxResult_e error);

MonitorErrorState_t MonitorTx_GetErrorState(const MonitorTx_Handle_t *htx);

/* Call from UART RX DMA/idle callback; chunks may be split or concatenated. */
void MonitorTx_ParseRxPacket(MonitorTx_Handle_t *htx,
                             const uint8_t *rx_data,
                             uint16_t length,
                             uint32_t current_time);

uint8_t MonitorTx_MakeStatusByte(const MonitorData_t *data);

#if defined(STM32F411xE)
/* tx_context must point to the UART_HandleTypeDef used for monitoring. */
MonitorTxResult_e MonitorTx_Stm32HalUartDmaStart(void *tx_context,
                                                 const uint8_t *data,
                                                 uint16_t length);
#endif

#ifdef __cplusplus
}
#endif