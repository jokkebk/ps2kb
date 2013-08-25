#ifndef __TIMER_H
#define __TIMER_H

#if !defined(__AVR_ATtiny2313__) && \
	!defined(__AVR_ATtiny45__) && \
	!defined(__AVR_ATtiny85__)
#error "Only ATtiny2313 and ATtiny45/85 are supported!"
#endif

#include <avr/io.h>

#define INT_VECT_0 TIMER0_COMPA_vect
#define INT_VECT_1 TIMER1_COMPA_vect

uint8_t startTimer_0(uint32_t hz);

uint8_t startTimer_1(uint32_t hz);

#endif