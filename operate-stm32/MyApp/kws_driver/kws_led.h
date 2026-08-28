#pragma once

#include "main.h"
#include <stdio.h>

typedef enum _system_state_t {
  NORMAL = 0,
  WARNING,
  DANGER,
  EMERGENCY_STOP
} system_state_t;

void update_ky016_oled(system_state_t mode);