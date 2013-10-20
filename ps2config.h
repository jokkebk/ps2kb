/**
 * PS/2 keyboard implementation and knock sensor.
 * PS/2 configuration.
 *
 * Copyright (C) Joonas Pihlajamaa 2013.
 * Licensed under GNU GPL v3, see License.txt for details.
 *
 * See details about this project at http://codeandlife.com/?p=1488
 */
#ifndef __PS2CONFIG_H
#define __PS2CONFIG_H

#include <avr/io.h>

#define PS2_CLOCK_DDR DDRB
#define PS2_CLOCK_PORT PORTB
#define PS2_CLOCK_PIN 0
#define PS2_CLOCK_INPUT PINB

#define PS2_DATA_DDR DDRB
#define PS2_DATA_PORT PORTB
#define PS2_DATA_PIN 1
#define PS2_DATA_INPUT PINB

// We can have either button press trigger space, or else
// ADC going above treshold (e.g. piezo vibration trigger)
#ifdef USE_BUTTON
#define BUTTON_PORT PORTB
#define BUTTON_PIN 3
#define BUTTON_DOWN() (!(PINB & _BV(BUTTON_PIN)))
#endif

// Optional caps lock (or other) LED
#ifdef LED_PIN
#define LED_PORT PORTB
#define LED_DDR DDRB
#define LED_MASK 4 // 4=caps 2=num 1=scroll
#endif

#endif
