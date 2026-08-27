#define RELAY_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>

#define RELAY_GPIO_PORT GPIOB
#define RELAY_GPIO_PIN GPIO_PIN_13
#define RELAY_ACTIVE_LEVEL GPIO_PIN_RESET

void Relay_Init(void);
bool Relay_SetMotorPower(bool on);
void Relay_EmergencyStop(void);
bool Relay_ClearEmergencyStop(void);
bool Relay_IsMotorPowerOn(void);
bool Relay_IsEmergencyStopped(void);