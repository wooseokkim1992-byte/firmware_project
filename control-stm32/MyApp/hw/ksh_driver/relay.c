#include "relay.h"

#include "main.h"
#include <stddef.h>

#define RELAY_GPIO_PORT    GPIOB
#define RELAY_GPIO_PIN     GPIO_PIN_13

static GPIO_PinState RelayPinLevel(const Relay_Handle_t *relay,
                                   RelayState_e state)
{
    bool high = (state == RELAY_STATE_ON);

    if (!relay->active_high) {
        high = !high;
    }
    return high ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

static void RelayApply(Relay_Handle_t *relay, RelayState_e state)
{
    relay->state = state;
    HAL_GPIO_WritePin(RELAY_GPIO_PORT,
                      RELAY_GPIO_PIN,
                      RelayPinLevel(relay, state));
}

void Relay_Init(Relay_Handle_t *relay, bool active_high)
{
    GPIO_InitTypeDef gpio = {0};

    if (relay == NULL) {
        return;
    }

    relay->active_high = active_high;
    relay->communication_connected = false;
    relay->emergency_stop_latched = false;
    relay->safety_reason = RELAY_SAFETY_BOOT;
    relay->state = RELAY_STATE_OFF;

    __HAL_RCC_GPIOB_CLK_ENABLE();
    HAL_GPIO_WritePin(RELAY_GPIO_PORT,
                      RELAY_GPIO_PIN,
                      RelayPinLevel(relay, RELAY_STATE_OFF));

    gpio.Pin = RELAY_GPIO_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(RELAY_GPIO_PORT, &gpio);
}

bool Relay_SetMotorPower(Relay_Handle_t *relay, bool turn_on)
{
    if (relay == NULL) {
        return false;
    }

    if (turn_on) {
        if (relay->emergency_stop_latched ||
            !relay->communication_connected) {
            RelayApply(relay, RELAY_STATE_OFF);
            return false;
        }
        relay->safety_reason = RELAY_SAFETY_NONE;
        RelayApply(relay, RELAY_STATE_ON);
        return true;
    }

    relay->safety_reason = relay->emergency_stop_latched
                               ? RELAY_SAFETY_EMERGENCY_STOP
                               : (relay->communication_connected
                                      ? RELAY_SAFETY_NONE
                                      : RELAY_SAFETY_COMMUNICATION_LOST);
    RelayApply(relay, RELAY_STATE_OFF);
    return true;
}

void Relay_EmergencyStop(Relay_Handle_t *relay)
{
    if (relay == NULL) {
        return;
    }

    relay->emergency_stop_latched = true;
    relay->safety_reason = RELAY_SAFETY_EMERGENCY_STOP;
    RelayApply(relay, RELAY_STATE_OFF);
}

void Relay_SetCommunicationConnected(Relay_Handle_t *relay, bool connected)
{
    if (relay == NULL) {
        return;
    }

    relay->communication_connected = connected;
    if (!connected) {
        relay->safety_reason = relay->emergency_stop_latched
                                   ? RELAY_SAFETY_EMERGENCY_STOP
                                   : RELAY_SAFETY_COMMUNICATION_LOST;
        RelayApply(relay, RELAY_STATE_OFF);
    } else if (!relay->emergency_stop_latched) {
        relay->safety_reason = RELAY_SAFETY_NONE;
    }
}

RelayState_e Relay_GetState(const Relay_Handle_t *relay)
{
    return (relay != NULL) ? relay->state : RELAY_STATE_OFF;
}

RelaySafetyReason_e Relay_GetSafetyReason(const Relay_Handle_t *relay)
{
    return (relay != NULL) ? relay->safety_reason : RELAY_SAFETY_BOOT;
}

bool Relay_IsEmergencyStopLatched(const Relay_Handle_t *relay)
{
    return (relay != NULL) ? relay->emergency_stop_latched : true;
}
