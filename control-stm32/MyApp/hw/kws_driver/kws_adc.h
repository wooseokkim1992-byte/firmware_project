#pragma once
#include "main.h"
#include <stdbool.h>
#include <stdint.h>

#define ADC_WINDOW_SIZE 800U
#define ADC_DMA_BUFFER_SIZE 1600U

extern uint16_t adc_dma_buffer[ADC_DMA_BUFFER_SIZE];
extern volatile bool adc_half_ready;
extern volatile bool adc_full_ready;
extern volatile uint32_t adc_overrun_count;

bool adc_init(void);
