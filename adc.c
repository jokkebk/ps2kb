/**
 * PS/2 keyboard implementation and knock sensor.
 * A/D conversion routines for ATtiny25/45/85.
 *
 * Copyright (C) Joonas Pihlajamaa 2013.
 * Licensed under GNU GPL v3, see License.txt for details.
 *
 * See details about this project at http://codeandlife.com/?p=1488
 */
#include "adc.h"

void adcStart() {
	// Initialize ADC
	// ADMUX REFS0:2 set to 0 means Vcc as voltage reference
	// ADMUX ADLAR also to zero so ADC value is right-adjusted (LSB)
	// ADMUX MUX[3:0] to 0011 to select PB3
	ADMUX = _BV(MUX1) + _BV(MUX0);

	// ADCSRB (ADC Control and Status Register B)
	ADCSRB = 0; // Free-running, not bipolar or reversed input polarity

	// ADCSRA (ADC Control and Status Register A)
	// ADPS[2:0] 110 to select prescale of 64 resulting in 125kHz @ 8MHz
	ADCSRA = _BV(ADPS2) + _BV(ADPS1);
	ADCSRA |= _BV(ADATE); // AD auto trigger enable (based on ADPS bits)
	ADCSRA |= _BV(ADEN); // ADC enable
	ADCSRA |= _BV(ADSC); // ADC start conversion
}
