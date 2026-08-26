#pragma once

#include <stdbool.h>
#include <stdint.h>

/* TIM3 counter should be configured to 10 kHz, Period = 65535. */

#define RPM_TIMER_FREQUENCY_HZ 10000UL
#define RPM_PULSES_PER_REVOLUTION 1UL
#define RPM_STOP_TIMEOUT_MS 3000UL
#define RPM_MIN_PULSE_INTERVAL_TICKS 20U

void RPM_Init(void);
void RPM_OnInputCapture(uint16_t captured_count, uint32_t now_ms);
void RPM_Update(uint32_t now_ms);
uint16_t RPM_Get(void);
bool RPM_IsRunning(void);
