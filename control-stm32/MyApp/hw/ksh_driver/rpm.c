#include "rpm.h"

static volatile uint16_t rpm_value;
static volatile uint16_t previous_capture;
static volatile uint32_t last_pulse_ms;
static volatile bool first_capture;
static volatile bool running;

void RPM_Init(void)
{
    rpm_value = 0U;
    previous_capture = 0U;
    last_pulse_ms = 0U;
    first_capture = true;
    running = false;
}

void RPM_OnInputCapture(uint16_t capture, uint32_t now_ms)
{
    uint16_t interval;
    uint32_t calculated;
    if (first_capture) {
        previous_capture = capture;
        first_capture = false;
        last_pulse_ms = now_ms;
        return;
    }

    interval = (uint16_t)(capture - previous_capture); /* Handles one 16-bit wrap. */
    if (interval < RPM_MIN_PULSE_INTERVAL_TICKS) return;
    previous_capture = capture;
    last_pulse_ms = now_ms;
    calculated = (60UL * RPM_TIMER_FREQUENCY_HZ) /
                 ((uint32_t)interval * RPM_PULSES_PER_REVOLUTION);
    if (calculated > 65535UL) calculated = 65535UL;
    /* Simple low-pass filter, while giving the first valid value immediately. */
    rpm_value = running ? (uint16_t)(((uint32_t)rpm_value * 3UL + calculated) / 4UL)
                        : (uint16_t)calculated;
    running = true;
}

void RPM_Update(uint32_t now_ms)
{
    if (running && ((uint32_t)(now_ms - last_pulse_ms) >= RPM_STOP_TIMEOUT_MS)) {
        rpm_value = 0U;
        running = false;
        first_capture = true;
    }
}

uint16_t RPM_Get(void) { return rpm_value; }
bool RPM_IsRunning(void) { return running; }