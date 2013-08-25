#ifndef __ADC_H
#define __ADC_H

#include <avr/io.h>

#define ADC_TRESHOLD 10

static inline uint16_t adcRead() {
	return ADC;
}

void adcStart();

#endif
