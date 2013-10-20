/**
 * PS/2 keyboard implementation and knock sensor.
 * A/D conversion routines for ATtiny25/45/85.
 *
 * Copyright (C) Joonas Pihlajamaa 2013.
 * Licensed under GNU GPL v3, see License.txt for details.
 *
 * See details about this project at http://codeandlife.com/?p=1488
 */
#ifndef __ADC_H
#define __ADC_H

#include <avr/io.h>

#define ADC_TRESHOLD 10

static inline uint16_t adcRead() {
	return ADC;
}

void adcStart();

#endif
