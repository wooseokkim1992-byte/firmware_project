#include "monitoring_tx.h"

#include <stddef.h>
#include <string.h>

static void SetConnected(MonitorTx_Handle_t *monitor, bool connected)
{
    monitor->error.connected = connected;
    Relay_SetCommunicationConnected(monitor->relay, connected);
}

static uint8_t MakeStatus(const MonitorTx_Handle_t *monitor,
                          const MonitorData_t *data)
{
    uint8_t status = (uint8_t)data->current_state & MONITOR_STATUS_STATE_MASK;

    status |= data->sensor_error_flags &
              (MONITOR_STATUS_VIBRATION_ERROR |
               MONITOR_STATUS_SOUND_ERROR);
    if (Relay_GetState(monitor->relay) == RELAY_STATE_ON) {
        status |= MONITOR_STATUS_RELAY_ON;
    }
    if (monitor->emergency_stop_latched) {
        status = (uint8_t)((status & 0xFCU) |
                           SYSTEM_STATE_EMERGENCY_STOP);
        status &= 0xBFU;
    }
    return status;
}

static void PutFloat(uint8_t *destination, float value)
{
    memcpy(destination, &value, sizeof(value));
}

static MonitorTxResult_e StartDma(MonitorTx_Handle_t *monitor,
                                  const MonitorData_t *data)
{
    uint8_t i;
    uint8_t checksum = 0U;
    HAL_StatusTypeDef result;

    if ((monitor == NULL) || (data == NULL) || (monitor->uart == NULL) ||
        (monitor->uart->hdmatx == NULL)) {
        return MONITOR_TX_ERROR;
    }
    if (monitor->error.tx_busy) {
        monitor->error.overflow_count++;
        return MONITOR_TX_BUFFER_OVERFLOW;
    }

    monitor->tx_buffer[0] = MONITOR_PACKET_STX;
    monitor->tx_buffer[1] = 9U;
    PutFloat(&monitor->tx_buffer[2], data->vibration_level);
    PutFloat(&monitor->tx_buffer[6], data->sound_level);
    monitor->tx_buffer[10] = MakeStatus(monitor, data);
    for (i = 1U; i <= 10U; i++) {
        checksum ^= monitor->tx_buffer[i];
    }
    monitor->tx_buffer[11] = checksum;

    monitor->in_flight_status = monitor->tx_buffer[10];
    monitor->error.tx_busy = true;
    result = HAL_UART_Transmit_DMA(monitor->uart,
                                   monitor->tx_buffer,
                                   MONITOR_PACKET_LENGTH);
    if (result == HAL_OK) {
        return MONITOR_TX_OK;
    }

    monitor->error.tx_busy = false;
    monitor->error.failure_count++;
    if (result == HAL_BUSY) {
        monitor->error.overflow_count++;
        return MONITOR_TX_BUFFER_OVERFLOW;
    }
    if (++monitor->error.consecutive_failures >=
        MONITOR_DISCONNECT_FAIL_COUNT) {
        SetConnected(monitor, false);
    }
    return MONITOR_TX_ERROR;
}

void MonitorTx_Init(MonitorTx_Handle_t *monitor,
                    UART_HandleTypeDef *uart,
                    Relay_Handle_t *relay,
                    uint32_t tx_interval_ms)
{
    if (monitor == NULL) {
        return;
    }

    memset(monitor, 0, sizeof(*monitor));
    monitor->uart = uart;
    monitor->relay = relay;
    monitor->tx_interval_ms = tx_interval_ms;
}

void MonitorTx_ProcessPeriodic(MonitorTx_Handle_t *monitor,
                               const MonitorData_t *data,
                               uint32_t current_time)
{
    uint8_t status;
    uint32_t elapsed;
    bool changed;

    if ((monitor == NULL) || (data == NULL)) {
        return;
    }

    if (monitor->has_received && monitor->error.connected &&
        ((current_time - monitor->last_rx_time) >=
         MONITOR_LINK_TIMEOUT_MS)) {
        SetConnected(monitor, false);
    }
    if (monitor->error.tx_busy) {
        return;
    }

    status = MakeStatus(monitor, data);
    elapsed = current_time - monitor->last_tx_time;
    changed = !monitor->has_sent || (status != monitor->last_sent_status);

    if ((!monitor->has_attempted || status != monitor->last_attempt_status) ||
        (changed && elapsed >= MONITOR_EVENT_RETRY_MS) ||
        (!changed && elapsed >= monitor->tx_interval_ms)) {
        monitor->last_attempt_status = status;
        monitor->last_tx_time = current_time;
        monitor->has_attempted = true;
        (void)StartDma(monitor, data);
    }
}

MonitorTxResult_e MonitorTx_SendEvent(MonitorTx_Handle_t *monitor,
                                      const MonitorData_t *data)
{
    return StartDma(monitor, data);
}

void MonitorTx_OnTxComplete(MonitorTx_Handle_t *monitor)
{
    if ((monitor == NULL) || !monitor->error.tx_busy) {
        return;
    }

    monitor->error.tx_busy = false;
    monitor->error.consecutive_failures = 0U;
    monitor->last_sent_status = monitor->in_flight_status;
    monitor->has_sent = true;
}

void MonitorTx_OnTxError(MonitorTx_Handle_t *monitor)
{
    if (monitor == NULL) {
        return;
    }

    monitor->error.tx_busy = false;
    monitor->error.failure_count++;
    if (++monitor->error.consecutive_failures >=
        MONITOR_DISCONNECT_FAIL_COUNT) {
        SetConnected(monitor, false);
    }
}

static void HandleCommand(MonitorTx_Handle_t *monitor,
                          uint32_t current_time)
{
    uint8_t command = monitor->rx_buffer[2];
    uint8_t checksum = monitor->rx_buffer[1] ^ command;

    if ((monitor->rx_buffer[1] != 1U) ||
        (monitor->rx_buffer[3] != checksum) ||
        ((command != MONITOR_COMMAND_HEARTBEAT) &&
         (command != MONITOR_COMMAND_EMERGENCY_STOP))) {
        monitor->error.invalid_rx_count++;
        return;
    }

    monitor->has_received = true;
    monitor->last_rx_time = current_time;
    SetConnected(monitor, true);

    if (command == MONITOR_COMMAND_EMERGENCY_STOP) {
        monitor->emergency_stop_latched = true;
        Relay_EmergencyStop(monitor->relay);
    }
}

void MonitorTx_ParseRxPacket(MonitorTx_Handle_t *monitor,
                             const uint8_t *data,
                             uint16_t length,
                             uint32_t current_time)
{
    uint16_t i;

    if ((monitor == NULL) || (data == NULL)) {
        return;
    }

    for (i = 0U; i < length; i++) {
        if (data[i] == MONITOR_PACKET_STX) {
            monitor->rx_buffer[0] = data[i];
            monitor->rx_length = 1U;
        } else if (monitor->rx_length > 0U) {
            monitor->rx_buffer[monitor->rx_length++] = data[i];
            if (monitor->rx_length == sizeof(monitor->rx_buffer)) {
                HandleCommand(monitor, current_time);
                monitor->rx_length = 0U;
            }
        }
    }
}

MonitorErrorState_t MonitorTx_GetErrorState(const MonitorTx_Handle_t *monitor)
{
    MonitorErrorState_t empty = {0};

    return (monitor != NULL) ? monitor->error : empty;
}
