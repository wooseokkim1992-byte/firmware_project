#pragma once

#include "relay.h"
#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <stdint.h>

#define MONITOR_PACKET_STX                 0x02U
#define MONITOR_COMMAND_HEARTBEAT           0xA5U
#define MONITOR_COMMAND_EMERGENCY_STOP      0xEEU
#define MONITOR_PACKET_LENGTH               12U
#define MONITOR_LINK_TIMEOUT_MS             45000U
#define MONITOR_EVENT_RETRY_MS              100U
#define MONITOR_DISCONNECT_FAIL_COUNT       10U

#define MONITOR_STATUS_STATE_MASK           0x03U
#define MONITOR_STATUS_VIBRATION_ERROR      0x10U
#define MONITOR_STATUS_SOUND_ERROR          0x20U
#define MONITOR_STATUS_RELAY_ON             0x40U

typedef enum {
    SYSTEM_STATE_NORMAL = 0,
    SYSTEM_STATE_WARNING,
    SYSTEM_STATE_DANGER,
    SYSTEM_STATE_EMERGENCY_STOP
} SystemState_e;

typedef enum {
    MONITOR_TX_OK = 0,
    MONITOR_TX_ERROR,
    MONITOR_TX_BUFFER_OVERFLOW
} MonitorTxResult_e;

typedef struct {
    float vibration_level;
    float sound_level;
    SystemState_e current_state;
    uint8_t sensor_error_flags;
} MonitorData_t;

typedef struct {
    uint16_t consecutive_failures;
    uint16_t failure_count;
    uint16_t overflow_count;
    uint16_t invalid_rx_count;
    bool connected;
    bool tx_busy;
} MonitorErrorState_t;

typedef struct {
    UART_HandleTypeDef *uart;
    Relay_Handle_t *relay;
    uint32_t tx_interval_ms;
    uint32_t last_tx_time;
    uint32_t last_rx_time;
    uint8_t last_sent_status;
    uint8_t last_attempt_status;
    uint8_t in_flight_status;
    uint8_t tx_buffer[MONITOR_PACKET_LENGTH];
    uint8_t rx_buffer[4];
    uint8_t rx_length;
    bool has_sent;
    bool has_attempted;
    bool has_received;
    bool emergency_stop_latched;
    MonitorErrorState_t error;
} MonitorTx_Handle_t;

void MonitorTx_Init(MonitorTx_Handle_t *monitor,
                    UART_HandleTypeDef *uart,
                    Relay_Handle_t *relay,
                    uint32_t tx_interval_ms);
void MonitorTx_ProcessPeriodic(MonitorTx_Handle_t *monitor,
                               const MonitorData_t *data,
                               uint32_t current_time);
MonitorTxResult_e MonitorTx_SendEvent(MonitorTx_Handle_t *monitor,
                                      const MonitorData_t *data);
void MonitorTx_OnTxComplete(MonitorTx_Handle_t *monitor);
void MonitorTx_OnTxError(MonitorTx_Handle_t *monitor);
void MonitorTx_ParseRxPacket(MonitorTx_Handle_t *monitor,
                             const uint8_t *data,
                             uint16_t length,
                             uint32_t current_time);
MonitorErrorState_t MonitorTx_GetErrorState(const MonitorTx_Handle_t *monitor);
