#ifndef __UTIL_H
#define __UTIL_H

#include <avr/io.h>

#define PS2_CLOCK_DDR DDRB
#define PS2_CLOCK_PORT PORTB
#define PS2_CLOCK_PIN 0
#define PS2_CLOCK_INPUT PINB

#define PS2_DATA_DDR DDRB
#define PS2_DATA_PORT PORTB
#define PS2_DATA_PIN 1
#define PS2_DATA_INPUT PINB

#define isClockHigh() (PS2_CLOCK_INPUT & (1 << PS2_CLOCK_PIN))
#define isDataHigh() (PS2_DATA_INPUT & (1 << PS2_DATA_PIN))

static inline void releaseClock() {
	PS2_CLOCK_DDR &= ~_BV(PS2_CLOCK_PIN); // set as input
	PS2_CLOCK_PORT |= _BV(PS2_CLOCK_PIN); // set pullup
}

static inline void holdClock() {
	PS2_CLOCK_PORT &= ~_BV(PS2_CLOCK_PIN); // zero output value
	PS2_CLOCK_DDR |= _BV(PS2_CLOCK_PIN); // set as output
}

static inline void releaseData() {
	PS2_DATA_DDR &= ~_BV(PS2_DATA_PIN); // set as input
	PS2_DATA_PORT |= _BV(PS2_DATA_PIN); // set pullup
}

static inline void holdData() {
	PS2_DATA_PORT &= ~_BV(PS2_DATA_PIN); // zero output value
	PS2_DATA_DDR |= _BV(PS2_DATA_PIN); // set as output
}

#endif
