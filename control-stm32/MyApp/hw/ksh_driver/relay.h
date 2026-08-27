#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    RELAY_STATE_OFF = 0,
    RELAY_STATE_ON = 1
} RelayState_e;

typedef enum {
    RELAY_SAFETY_BOOT = 0,
    RELAY_SAFETY_NONE,
    RELAY_SAFETY_EMERGENCY_STOP,
    RELAY_SAFETY_COMMUNICATION_LOST
} RelaySafetyReason_e;

/* gpio_init configures the relay pin as a digital output at initial_pin_level. */
typedef bool (*RelayGpioInit_Function_t)(void *context,
                                        bool initial_pin_level);
typedef void (*RelayGpioWrite_Function_t)(void *context, bool pin_level);
typedef void (*RelayStateChanged_Callback_t)(void *context,
                                             RelayState_e state,
                                             RelaySafetyReason_e reason);

typedef struct {
    bool active_high;
    RelayState_e state;
    RelaySafetyReason_e safety_reason;
    bool emergency_stop_latched;
    bool communication_connected;
    bool gpio_initialized;

    RelayGpioInit_Function_t GpioInit_Function;
    RelayGpioWrite_Function_t GpioWrite_Function;
    void *gpio_context;
    RelayStateChanged_Callback_t StateChanged_Callback;
    void *state_changed_context;
} Relay_Handle_t;

/*
 * Init configures the GPIO through gpio_init and immediately applies Relay
 * OFF, which is the safe output for boot/reset.
 */
bool Relay_Init(Relay_Handle_t *hrelay,
                bool active_high,
                RelayGpioInit_Function_t gpio_init,
                RelayGpioWrite_Function_t gpio_write,
                void *gpio_context);

void Relay_SetStateChangedCallback(Relay_Handle_t *hrelay,
                                   RelayStateChanged_Callback_t callback,
                                   void *context);

/* ON is rejected while E-stop is latched or communication is disconnected. */
bool Relay_SetMotorPower(Relay_Handle_t *hrelay, bool turn_on);

/* Signature is directly compatible with MonitorTx's E-stop callback. */
void Relay_EmergencyStopCallback(void *context);

/* Signature is directly compatible with MonitorTx's link-state callback. */
void Relay_CommunicationStateCallback(void *context, bool connected);

RelayState_e Relay_GetState(const Relay_Handle_t *hrelay);
RelaySafetyReason_e Relay_GetSafetyReason(const Relay_Handle_t *hrelay);
bool Relay_IsEmergencyStopLatched(const Relay_Handle_t *hrelay);

#if defined(STM32F411xE)
/* Nucleo-F411RE board mapping: PB13, push-pull digital output. */
bool Relay_InitNucleoF411RE_PB13(Relay_Handle_t *hrelay,
                                 bool active_high);
#endif

#ifdef __cplusplus
}
#endif