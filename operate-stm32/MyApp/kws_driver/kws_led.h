#pragma once

#include "kws_display_type.h"
#include "main.h"
#include <stdio.h>

#define VIBE_GPIO_R_PORT GPIOB;
#define VIBE_GPIO_G_PORT GPIOB;
#define VIBE_GPIO_B_PORT GPIOB;
#define VIBE_GPIO_R_PIN GPIO_PIN_10;
#define VIBE_GPIO_G_PIN GPIO_PIN_4;
#define VIBE_GPIO_B_PIN GPIO_PIN_5;

#define SOUND_GPIO_R_PORT GPIOB;
#define SOUND_GPIO_G_PORT GPIOC;
#define SOUND_GPIO_B_PORT GPIOC;
#define SOUND_GPIO_R_PIN GPIO_PIN_0;
#define SOUND_GPIO_G_PIN GPIO_PIN_1;
#define SOUND_GPIO_B_PIN GPIO_PIN_0;

void update_ky016_oled(system_state_t mode);

void update_ky016_oled_1(system_state_t mode, GPIO_TypeDef *GPIOx_R, uint16_t R,
                         GPIO_TypeDef *GPIOx_G, uint16_t G,
                         GPIO_TypeDef *GPIOx_B, uint16_t B);