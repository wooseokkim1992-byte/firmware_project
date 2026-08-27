#include "relay.h"

static volatile bool motor_power_on;
static volatile bool emergency_latched;

static GPIO_PinState inactive_level(void)
{
    return (RELAY_ACTIVE_LEVEL == GPIO_PIN_SET) ? GPIO_PIN_RESET : GPIO_PIN_SET;
}

void Relay_Init(void)
{
    /* MX_GPIO_Init() must run first. Always start with motor power removed. */
    HAL_GPIO_WritePin(RELAY_GPIO_PORT, RELAY_GPIO_PIN, inactive_level());
    motor_power_on = false;
    emergency_latched = false;
}

bool Relay_SetMotorPower(bool on)
{
    if (on && emergency_latched) return false;
    HAL_GPIO_WritePin(RELAY_GPIO_PORT, RELAY_GPIO_PIN,
                     on ? RELAY_ACTIVE_LEVEL : inactive_level());
    motor_power_on = on;
    return true;
}

void Relay_EmergencyStop(void)
{
    emergency_latched = true;
    HAL_GPIO_WritePin(RELAY_GPIO_PORT, RELAY_GPIO_PIN, inactive_level());
    motor_power_on = false;
}

bool Relay_ClearEmergencyStop(void)
{
    /* Clearing the latch never restarts the motor automatically. */
    emergency_latched = false;
    return true;
}

bool Relay_IsMotorPowerOn(void) { return motor_power_on; }
bool Relay_IsEmergencyStopped(void) { return emergency_latched; }