#pragma once
#include "main.h"
#include <stdbool.h>

#define ADC_WINDOW_SIZE 800U
#define ADC_DMA_BUFFER_SIZE 1600U

bool adc_init(void);
void get_adc_dma_data_half(void);
