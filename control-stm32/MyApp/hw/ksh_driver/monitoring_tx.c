#include "monitoring_tx.h"

#include <limits.h>
#include <stddef.h>
#include <string.h>

#if defined(STM32F411xE)
#include "stm32f4xx_hal.h"
#endif

static void IncrementSaturatedU16(uint16_t *value)
{
    if (*value < UINT16_MAX) {
        (*value)++;
    }
}

static void IncrementSaturatedU32(uint32_t *value)
{
    if (*value < UINT32_MAX) {
        (*value)++;
    }
}

static void SetConnectionState(MonitorTx_Handle_t *htx, bool connected)
{
    bool changed = (htx->error_status.is_connected != connected);

    htx->error_status.is_connected = connected;
    if (changed && (htx->LinkState_Callback != NULL)) {
        htx->LinkState_Callback(htx->link_state_context, connected);
    }
}

static void RecordTxError(MonitorTx_Handle_t *htx,
                          MonitorTxResult_e error)
{
    if ((htx == NULL) || (error == MONITOR_TX_OK)) {
        return;
    }

    htx->error_status.last_tx_result = error;
    IncrementSaturatedU32(&htx->error_status.tx_failure_total);

    if (error == MONITOR_TX_BUFFER_OVERFLOW) {
        htx->error_status.buffer_overflow = true;
        IncrementSaturatedU32(&htx->error_status.tx_buffer_overflow_total);
        return;
    }

    IncrementSaturatedU16(&htx->error_status.tx_fail_count);

    if ((error == MONITOR_TX_DISCONNECTED) ||
        (htx->error_status.tx_fail_count >= MONITOR_DISCONNECT_FAIL_COUNT)) {
        SetConnectionState(htx, false);
    }
}

static void StoreFloatLittleEndian(uint8_t *destination, float value)
{
    uint8_t bytes[sizeof(float)];
    uint16_t i;

    memcpy(bytes, &value, sizeof(bytes));

#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    for (i = 0u; i < (uint16_t)sizeof(bytes); i++) {
        destination[i] = bytes[(sizeof(bytes) - 1u) - i];
    }
#else
    (void)i;
    memcpy(destination, bytes, sizeof(bytes));
#endif
}

uint8_t MonitorTx_MakeStatusByte(const MonitorData_t *data)
{
    uint8_t status;

    if (data == NULL) {
        return 0u;
    }

    status = (uint8_t)data->current_state & MONITOR_STATUS_STATE_MASK;
    status |= data->sensor_error_flags & MONITOR_SENSOR_ERROR_MASK;
    if (data->relay_on) {
        status |= MONITOR_STATUS_RELAY_ON;
    }

    return status;
}

static uint8_t MakeEffectiveStatusByte(const MonitorTx_Handle_t *htx,
                                       const MonitorData_t *data)
{
    uint8_t status = MonitorTx_MakeStatusByte(data);

    if ((htx != NULL) && htx->emergency_stop_latched) {
        status = (uint8_t)(status & 0xFCu);
        status |= (uint8_t)SYSTEM_STATE_EMERGENCY_STOP;
        status = (uint8_t)(status & 0xBFu);
    }
    return status;
}

static MonitorTxResult_e BuildAndStartDma(MonitorTx_Handle_t *htx,
                                          const MonitorData_t *data)
{
    uint8_t index = 0u;
    uint8_t checksum = 0u;
    uint8_t i;
    MonitorTxResult_e result;

    if ((htx == NULL) || (data == NULL) || (htx->Tx_Function == NULL)) {
        return MONITOR_TX_FAILED;
    }

    if (htx->error_status.tx_in_progress) {
        RecordTxError(htx, MONITOR_TX_BUFFER_OVERFLOW);
        return MONITOR_TX_BUFFER_OVERFLOW;
    }

    htx->tx_buffer[index++] = MONITOR_PACKET_STX;
    htx->tx_buffer[index++] = MONITOR_TELEMETRY_PAYLOAD_LENGTH;
    StoreFloatLittleEndian(&htx->tx_buffer[index], data->vibration_level);
    index += (uint8_t)sizeof(float);
    StoreFloatLittleEndian(&htx->tx_buffer[index], data->sound_level);
    index += (uint8_t)sizeof(float);
    htx->tx_buffer[index++] = MakeEffectiveStatusByte(htx, data);

    for (i = 1u; i < index; i++) {
        checksum ^= htx->tx_buffer[i];
    }
    htx->tx_buffer[index++] = checksum;

    if (index != MONITOR_TELEMETRY_PACKET_LENGTH) {
        RecordTxError(htx, MONITOR_TX_BUFFER_OVERFLOW);
        return MONITOR_TX_BUFFER_OVERFLOW;
    }

    htx->in_flight_status = htx->tx_buffer[10];
    htx->error_status.tx_in_progress = true;
    result = htx->Tx_Function(htx->tx_context, htx->tx_buffer, index);
    htx->error_status.last_tx_result = result;
    if (result != MONITOR_TX_OK) {
        htx->error_status.tx_in_progress = false;
        RecordTxError(htx, result);
        return result;
    }

    return MONITOR_TX_OK;
}

void MonitorTx_Init(MonitorTx_Handle_t *htx,
                    uint32_t interval_ms,
                    MonitorTx_Function_t tx_func,
                    void *tx_context)
{
    if (htx == NULL) {
        return;
    }

    memset(htx, 0, sizeof(*htx));
    htx->tx_interval_ms = interval_ms;
    htx->link_timeout_ms = MONITOR_DEFAULT_LINK_TIMEOUT_MS;
    htx->Tx_Function = tx_func;
    htx->tx_context = tx_context;
    htx->error_status.last_tx_result = MONITOR_TX_DISCONNECTED;
}

void MonitorTx_SetLinkTimeout(MonitorTx_Handle_t *htx,
                              uint32_t timeout_ms)
{
    if ((htx == NULL) || (timeout_ms == 0u)) {
        return;
    }

    htx->link_timeout_ms = timeout_ms;
}

void MonitorTx_SetEmergencyStopCallback(
    MonitorTx_Handle_t *htx,
    MonitorEmergencyStop_Callback_t callback,
    void *context)
{
    if (htx == NULL) {
        return;
    }

    htx->EmergencyStop_Callback = callback;
    htx->emergency_stop_context = context;
}

void MonitorTx_SetLinkStateCallback(MonitorTx_Handle_t *htx,
                                    MonitorLinkState_Callback_t callback,
                                    void *context)
{
    if (htx == NULL) {
        return;
    }

    htx->LinkState_Callback = callback;
    htx->link_state_context = context;
}

static void CheckLinkTimeout(MonitorTx_Handle_t *htx,
                             uint32_t current_time)
{
    if (htx->has_link_activity && htx->error_status.is_connected &&
        ((current_time - htx->last_link_activity_time) >=
         htx->link_timeout_ms)) {
        SetConnectionState(htx, false);
    }
}

void MonitorTx_ProcessPeriodic(MonitorTx_Handle_t *htx,
                               const MonitorData_t *data,
                               uint32_t current_time)
{
    uint8_t status;
    bool event_pending;
    bool new_event;
    uint32_t elapsed;

    if ((htx == NULL) || (data == NULL)) {
        return;
    }

    CheckLinkTimeout(htx, current_time);
    if (htx->error_status.tx_in_progress) {
        return;
    }

    status = MakeEffectiveStatusByte(htx, data);
    event_pending = (!htx->has_sent_packet || (status != htx->last_sent_status));
    new_event = (!htx->has_attempted_packet ||
                 (status != htx->last_attempt_status));
    elapsed = current_time - htx->last_tx_attempt_time;

    if (event_pending) {
        if (new_event || (elapsed >= MONITOR_EVENT_RETRY_INTERVAL_MS)) {
            htx->last_attempt_status = status;
            htx->last_tx_attempt_time = current_time;
            htx->has_attempted_packet = true;
            (void)BuildAndStartDma(htx, data);
        }
        return;
    }

    if (elapsed >= htx->tx_interval_ms) {
        htx->last_attempt_status = status;
        htx->last_tx_attempt_time = current_time;
        htx->has_attempted_packet = true;
        (void)BuildAndStartDma(htx, data);
    }
}

MonitorTxResult_e MonitorTx_SendEvent(MonitorTx_Handle_t *htx,
                                      const MonitorData_t *data)
{
    if ((htx == NULL) || (data == NULL)) {
        return MONITOR_TX_FAILED;
    }

    htx->last_attempt_status = MakeEffectiveStatusByte(htx, data);
    htx->has_attempted_packet = true;
    return BuildAndStartDma(htx, data);
}

void MonitorTx_OnTxComplete(MonitorTx_Handle_t *htx)
{
    if ((htx == NULL) || !htx->error_status.tx_in_progress) {
        return;
    }

    htx->error_status.tx_in_progress = false;
    htx->error_status.tx_fail_count = 0u;
    htx->error_status.buffer_overflow = false;
    htx->error_status.last_tx_result = MONITOR_TX_OK;
    htx->last_sent_status = htx->in_flight_status;
    htx->has_sent_packet = true;
}

void MonitorTx_OnTxError(MonitorTx_Handle_t *htx,
                         MonitorTxResult_e error)
{
    if (htx == NULL) {
        return;
    }

    htx->error_status.tx_in_progress = false;
    if (error == MONITOR_TX_OK) {
        error = MONITOR_TX_FAILED;
    }
    RecordTxError(htx, error);
}

MonitorErrorState_t MonitorTx_GetErrorState(const MonitorTx_Handle_t *htx)
{
    MonitorErrorState_t empty_state;

    memset(&empty_state, 0, sizeof(empty_state));
    empty_state.last_tx_result = MONITOR_TX_DISCONNECTED;
    if (htx == NULL) {
        return empty_state;
    }

    return htx->error_status;
}

static void ResetRxBuffer(MonitorTx_Handle_t *htx)
{
    htx->rx_length = 0u;
}

static void HandleCompleteRxFrame(MonitorTx_Handle_t *htx,
                                  uint32_t current_time)
{
    uint16_t expected_length;
    uint16_t i;
    uint8_t checksum = 0u;
    uint8_t received_checksum;

    expected_length = (uint16_t)htx->rx_buffer[1] + 3u;
    received_checksum = htx->rx_buffer[expected_length - 1u];

    for (i = 1u; i < (expected_length - 1u); i++) {
        checksum ^= htx->rx_buffer[i];
    }

    if (checksum != received_checksum) {
        IncrementSaturatedU32(&htx->error_status.rx_invalid_packet_total);
        return;
    }

    htx->has_link_activity = true;
    htx->last_link_activity_time = current_time;
    SetConnectionState(htx, true);

    if ((htx->rx_buffer[1] == 1u) &&
        (htx->rx_buffer[2] == MONITOR_COMMAND_EMERGENCY_STOP)) {
        htx->emergency_stop_latched = true;
        if (htx->EmergencyStop_Callback != NULL) {
            htx->EmergencyStop_Callback(htx->emergency_stop_context);
        }
    }
}

void MonitorTx_ParseRxPacket(MonitorTx_Handle_t *htx,
                             const uint8_t *rx_data,
                             uint16_t length,
                             uint32_t current_time)
{
    uint16_t i;

    if ((htx == NULL) || (rx_data == NULL)) {
        return;
    }

    for (i = 0u; i < length; i++) {
        uint16_t expected_length;

        if (htx->rx_length == 0u) {
            if (rx_data[i] == MONITOR_PACKET_STX) {
                htx->rx_buffer[htx->rx_length++] = rx_data[i];
            }
            continue;
        }

        if (htx->rx_length >= MONITOR_RX_BUFFER_SIZE) {
            htx->error_status.buffer_overflow = true;
            IncrementSaturatedU32(&htx->error_status.rx_buffer_overflow_total);
            ResetRxBuffer(htx);
            if (rx_data[i] == MONITOR_PACKET_STX) {
                htx->rx_buffer[htx->rx_length++] = rx_data[i];
            }
            continue;
        }

        htx->rx_buffer[htx->rx_length++] = rx_data[i];
        if (htx->rx_length < 2u) {
            continue;
        }

        expected_length = (uint16_t)htx->rx_buffer[1] + 3u;
        if (expected_length < 4u) {
            IncrementSaturatedU32(&htx->error_status.rx_invalid_packet_total);
            ResetRxBuffer(htx);
            continue;
        }

        if (expected_length > MONITOR_RX_BUFFER_SIZE) {
            htx->error_status.buffer_overflow = true;
            IncrementSaturatedU32(&htx->error_status.rx_buffer_overflow_total);
            ResetRxBuffer(htx);
            continue;
        }

        if (htx->rx_length == expected_length) {
            HandleCompleteRxFrame(htx, current_time);
            ResetRxBuffer(htx);
        }
    }
}

#if defined(STM32F411xE)
MonitorTxResult_e MonitorTx_Stm32HalUartDmaStart(void *tx_context,
                                                 const uint8_t *data,
                                                 uint16_t length)
{
    UART_HandleTypeDef *huart = (UART_HandleTypeDef *)tx_context;
    HAL_StatusTypeDef status;

    if ((huart == NULL) || (data == NULL) || (length == 0u)) {
        return MONITOR_TX_FAILED;
    }

    status = HAL_UART_Transmit_DMA(huart, (uint8_t *)data, length);
    if (status == HAL_OK) {
        return MONITOR_TX_OK;
    }
    if (status == HAL_BUSY) {
        return MONITOR_TX_BUFFER_OVERFLOW;
    }
    return MONITOR_TX_FAILED;
}
#endif