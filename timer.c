/**
 * PS/2 keyboard implementation and knock sensor.
 * Timer routines.
 *
 * Copyright (C) Joonas Pihlajamaa 2013.
 * Licensed under GNU GPL v3, see LICENSE for details.
 *
 * See README.md or http://codeandlife/?p=1488 for details.
 */
#include <avr/pgmspace.h>

#include "timer.h"

#define PRESCALES_0 5

static prog_uint8_t prescaleBits_0[PRESCALES_0] = {
	0 * _BV(CS02) + 0 * _BV(CS01) + 1 * _BV(CS00),
	0 * _BV(CS02) + 1 * _BV(CS01) + 0 * _BV(CS00),
	0 * _BV(CS02) + 1 * _BV(CS01) + 1 * _BV(CS00),
	1 * _BV(CS02) + 0 * _BV(CS01) + 0 * _BV(CS00),
	1 * _BV(CS02) + 0 * _BV(CS01) + 1 * _BV(CS00)
}; 

static prog_uint16_t prescales_0[PRESCALES_0] = { 
	1, 8, 64, 256, 1024 
};

uint8_t startTimer_0(uint32_t hz) {
	uint32_t clocks = F_CPU / hz, ticks;
	uint16_t prescale;
	uint8_t i, prescaleBits;

	// Init timer0 (50 kHz clock generation / logic)
	TIMSK |= (1 << OCIE0A); // timer 0 (8-bit) output compare interrupt
	TCCR0A = (1 << WGM01); // WGM02:0 = 2, Clear Timer on Count

	for(i = 0; i < PRESCALES_0; i++) {
		prescale = pgm_read_word(&prescales_0[i]);
		prescaleBits = pgm_read_byte(&prescaleBits_0[i]);

		if(clocks / prescale < 256) {
			OCR0A = (clocks / prescale) - 1;
			TCCR0B = prescaleBits;
			return 1; // SUCCESS
		}
	}

	return 0; // FAIL
}

#if defined(__AVR_ATtiny2313__) // uses 16-bit timer/counter 1

#define PRESCALES_1 5

static prog_uint8_t prescaleBits_1[PRESCALES_1] = {
	0 * _BV(CS12) + 0 * _BV(CS11) + 1 * _BV(CS10),
	0 * _BV(CS12) + 1 * _BV(CS11) + 0 * _BV(CS10),
	0 * _BV(CS12) + 1 * _BV(CS11) + 1 * _BV(CS10),
	1 * _BV(CS12) + 0 * _BV(CS11) + 0 * _BV(CS10),
	1 * _BV(CS12) + 0 * _BV(CS11) + 1 * _BV(CS10)
}; 

static prog_uint16_t prescales_1[PRESCALES_1] = { 
	1, 8, 64, 256, 1024
};

#elif defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny45__)

#define PRESCALES_1 10 // just the first few for now

// By default we should be in synchronous mode with ATtiny45
// so these bits are relative to system clock, not peripheral
static prog_uint8_t prescaleBits_1[PRESCALES_1] = {
	0 * _BV(CS12) + 0 * _BV(CS11) + 1 * _BV(CS10),
	0 * _BV(CS12) + 1 * _BV(CS11) + 0 * _BV(CS10),
	0 * _BV(CS12) + 1 * _BV(CS11) + 1 * _BV(CS10),
	1 * _BV(CS12) + 0 * _BV(CS11) + 0 * _BV(CS10),
	1 * _BV(CS12) + 0 * _BV(CS11) + 1 * _BV(CS10),
	1 * _BV(CS12) + 1 * _BV(CS11) + 0 * _BV(CS10),
	1 * _BV(CS12) + 1 * _BV(CS11) + 1 * _BV(CS10),
	_BV(CS13), _BV(CS13) + _BV(CS10), _BV(CS13) + _BV(CS11) 
}; 

static prog_uint16_t prescales_1[PRESCALES_1] = { 
	1, 2, 4, 8, 16, 32, 64, 128, 256, 512
};

#endif

uint8_t startTimer_1(uint32_t hz) {
	uint32_t clocks = F_CPU / hz, ticks;
	uint16_t prescale;
	uint8_t i, prescaleBits;

	// Init timer0 (50 kHz clock generation / logic)
	TIMSK |= (1 << OCIE1A); // timer 1 (8-bit) output compare interrupt

	for(i = 0; i < PRESCALES_1; i++) {
		prescale = pgm_read_word(&prescales_1[i]);
		prescaleBits = pgm_read_byte(&prescaleBits_1[i]);

		if(clocks / prescale < 256) {
			OCR1A = (clocks / prescale) - 1;
#if defined(__AVR_ATtiny2313__) // uses 16-bit timer/counter 1
			TCCR1B = _BV(WGM12) + prescaleBits;
#elif defined(__AVR_ATtiny85__) || defined(__AVR_ATtiny45__)
			TCCR1 = _BV(CTC1) + prescaleBits;
#endif
			return 1; // SUCCESS
		}
	}

	return 0; // FAIL
}


