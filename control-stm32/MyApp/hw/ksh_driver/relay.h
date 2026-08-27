#pragma once

#include <stdbool.h>

typedef enum {
    RELAY_STATE_OFF = 0,
    RELAY_STATE_ON
} RelayState_e;

typedef enum {
    RELAY_SAFETY_BOOT = 0,
    RELAY_SAFETY_NONE,
    RELAY_SAFETY_EMERGENCY_STOP,
    RELAY_SAFETY_COMMUNICATION_LOST
} RelaySafetyReason_e;

typedef struct {
    RelayState_e state;
    RelaySafetyReason_e safety_reason;
    bool active_high;
    bool communication_connected;
    bool emergency_stop_latched;
} Relay_Handle_t;

/* Nucleo-F411RE: PB13. active_high is fixed after checking the relay module. */
void Relay_Init(Relay_Handle_t *relay, bool active_high);
bool Relay_SetMotorPower(Relay_Handle_t *relay, bool turn_on);
void Relay_EmergencyStop(Relay_Handle_t *relay);
void Relay_SetCommunicationConnected(Relay_Handle_t *relay, bool connected);
RelayState_e Relay_GetState(const Relay_Handle_t *relay);
RelaySafetyReason_e Relay_GetSafetyReason(const Relay_Handle_t *relay);
bool Relay_IsEmergencyStopLatched(const Relay_Handle_t *relay);
