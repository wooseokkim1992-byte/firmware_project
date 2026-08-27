#include "relay.h"

#include <stddef.h>
#include <string.h>

#if defined(STM32F411xE)
#include "stm32f4xx_hal.h"
#endif

static void ApplyRelayOutput(Relay_Handle_t *hrelay, RelayState_e state)
{
    bool pin_level;

    hrelay->state = state;
    pin_level = (state == RELAY_STATE_ON);
    if (!hrelay->active_high) {
        pin_level = !pin_level;
    }

    if (hrelay->GpioWrite_Function != NULL) {
        hrelay->GpioWrite_Function(hrelay->gpio_context, pin_level);
    }
}

static void NotifyStateIfChanged(Relay_Handle_t *hrelay,
                                 RelayState_e previous_state,
                                 RelaySafetyReason_e previous_reason)
{
    if ((hrelay->StateChanged_Callback != NULL) &&
        ((previous_state != hrelay->state) ||
         (previous_reason != hrelay->safety_reason))) {
        hrelay->StateChanged_Callback(hrelay->state_changed_context,
                                      hrelay->state,
                                      hrelay->safety_reason);
    }
}

bool Relay_Init(Relay_Handle_t *hrelay,
                bool active_high,
                RelayGpioInit_Function_t gpio_init,
                RelayGpioWrite_Function_t gpio_write,
                void *gpio_context)
{
    bool safe_pin_level;

    if (hrelay == NULL) {
        return false;
    }

    memset(hrelay, 0, sizeof(*hrelay));
    hrelay->active_high = active_high;
    hrelay->GpioInit_Function = gpio_init;
    hrelay->GpioWrite_Function = gpio_write;
    hrelay->gpio_context = gpio_context;
    hrelay->safety_reason = RELAY_SAFETY_BOOT;
    safe_pin_level = !active_high;

    if ((gpio_init == NULL) || (gpio_write == NULL) ||
        !gpio_init(gpio_context, safe_pin_level)) {
        return false;
    }

    hrelay->gpio_initialized = true;
    ApplyRelayOutput(hrelay, RELAY_STATE_OFF);
    return true;
}

void Relay_SetStateChangedCallback(Relay_Handle_t *hrelay,
                                   RelayStateChanged_Callback_t callback,
                                   void *context)
{
    if (hrelay == NULL) {
        return;
    }

    hrelay->StateChanged_Callback = callback;
    hrelay->state_changed_context = context;
}

bool Relay_SetMotorPower(Relay_Handle_t *hrelay, bool turn_on)
{
    RelayState_e previous_state;
    RelaySafetyReason_e previous_reason;

    if ((hrelay == NULL) || !hrelay->gpio_initialized) {
        return false;
    }

    previous_state = hrelay->state;
    previous_reason = hrelay->safety_reason;

    if (turn_on) {
        if (hrelay->emergency_stop_latched ||
            !hrelay->communication_connected) {
            ApplyRelayOutput(hrelay, RELAY_STATE_OFF);
            return false;
        }
        hrelay->safety_reason = RELAY_SAFETY_NONE;
        ApplyRelayOutput(hrelay, RELAY_STATE_ON);
    } else {
        if (hrelay->emergency_stop_latched) {
            hrelay->safety_reason = RELAY_SAFETY_EMERGENCY_STOP;
        } else if (!hrelay->communication_connected) {
            hrelay->safety_reason = RELAY_SAFETY_COMMUNICATION_LOST;
        } else {
            hrelay->safety_reason = RELAY_SAFETY_NONE;
        }
        ApplyRelayOutput(hrelay, RELAY_STATE_OFF);
    }

    NotifyStateIfChanged(hrelay, previous_state, previous_reason);
    return true;
}

void Relay_EmergencyStopCallback(void *context)
{
    Relay_Handle_t *hrelay = (Relay_Handle_t *)context;
    RelayState_e previous_state;
    RelaySafetyReason_e previous_reason;

    if (hrelay == NULL) {
        return;
    }

    previous_state = hrelay->state;
    previous_reason = hrelay->safety_reason;
    hrelay->emergency_stop_latched = true;
    hrelay->safety_reason = RELAY_SAFETY_EMERGENCY_STOP;
    ApplyRelayOutput(hrelay, RELAY_STATE_OFF);
    NotifyStateIfChanged(hrelay, previous_state, previous_reason);
}

void Relay_CommunicationStateCallback(void *context, bool connected)
{
    Relay_Handle_t *hrelay = (Relay_Handle_t *)context;
    RelayState_e previous_state;
    RelaySafetyReason_e previous_reason;

    if (hrelay == NULL) {
        return;
    }

    previous_state = hrelay->state;
    previous_reason = hrelay->safety_reason;
    hrelay->communication_connected = connected;

    if (!connected) {
        hrelay->safety_reason = hrelay->emergency_stop_latched
                                    ? RELAY_SAFETY_EMERGENCY_STOP
                                    : RELAY_SAFETY_COMMUNICATION_LOST;
        ApplyRelayOutput(hrelay, RELAY_STATE_OFF);
    } else if (!hrelay->emergency_stop_latched &&
               ((hrelay->safety_reason == RELAY_SAFETY_BOOT) ||
                (hrelay->safety_reason == RELAY_SAFETY_COMMUNICATION_LOST))) {
        hrelay->safety_reason = RELAY_SAFETY_NONE;
        ApplyRelayOutput(hrelay, RELAY_STATE_OFF);
    }

    NotifyStateIfChanged(hrelay, previous_state, previous_reason);
}

RelayState_e Relay_GetState(const Relay_Handle_t *hrelay)
{
    return (hrelay != NULL) ? hrelay->state : RELAY_STATE_OFF;
}

RelaySafetyReason_e Relay_GetSafetyReason(const Relay_Handle_t *hrelay)
{
    return (hrelay != NULL) ? hrelay->safety_reason : RELAY_SAFETY_BOOT;
}

bool Relay_IsEmergencyStopLatched(const Relay_Handle_t *hrelay)
{
    return (hrelay != NULL) ? hrelay->emergency_stop_latched : true;
}

#if defined(STM32F411xE)
static bool NucleoF411RE_PB13_Init(void *context, bool initial_pin_level)
{
    GPIO_InitTypeDef gpio = { 0 };

    (void)context;
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* Set the safe level before switching PB13 into output mode. */
    HAL_GPIO_WritePin(GPIOB,
                      GPIO_PIN_13,
                      initial_pin_level ? GPIO_PIN_SET : GPIO_PIN_RESET);

    gpio.Pin = GPIO_PIN_13;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &gpio);
    return true;
}

static void NucleoF411RE_PB13_Write(void *context, bool pin_level)
{
    (void)context;
    HAL_GPIO_WritePin(GPIOB,
                      GPIO_PIN_13,
                      pin_level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

bool Relay_InitNucleoF411RE_PB13(Relay_Handle_t *hrelay,
                                 bool active_high)
{
    return Relay_Init(hrelay,
                      active_high,
                      NucleoF411RE_PB13_Init,
                      NucleoF411RE_PB13_Write,
                      NULL);
}
#endif
