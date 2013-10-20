/**
 * PS/2 keyboard implementation and knock sensor.
 * PS/2 implementation.
 *
 * Copyright (C) Joonas Pihlajamaa 2013.
 * Licensed under GNU GPL v3, see LICENSE for details.
 *
 * See README.md or http://codeandlife/?p=1488 for details.
 */
#ifndef __PS2_H
#define __PS2_H

#include "ps2config.h"
#include "ring.h"

#define PS2_LED_SCROLL_LOCK 1
#define PS2_LED_NUM_LOCK 2
#define PS2_LED_CAPS_LOCK 4

typedef enum {
	PS2ERROR_NONE = 0,
	PS2ERROR_INTERRUPTED = 1
} PS2Error;

// Free-running milliseconds counter
extern volatile uint16_t millis;
extern volatile PS2Error ps2Error;

// PS/2 send and receive buffers
extern volatile RingBuffer sendBuffer, receiveBuffer;

void initPS2();

#define isClockHigh() (PS2_CLOCK_INPUT & (1 << PS2_CLOCK_PIN))
#define isClockLow() (!isClockHigh())

#define isDataHigh() (PS2_DATA_INPUT & (1 << PS2_DATA_PIN))
#define isDataLow() (!isDataHigh())

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

#define PS2_Receive_Error 0
#define IS_PS2_CMD(cmd) ((cmd) >= PS2_CMD_Echo)

// PS/2 commands
#define PS2_CMD_Reset 0xFF
#define PS2_CMD_Resend 0xFE
#define PS2_CMD_Set_Key_Type_Make 0xFD
#define PS2_CMD_Set_Key_Type_Make_Break 0xFC
#define PS2_CMD_Set_Key_Type_Typematic 0xFB
#define PS2_CMD_Set_All_Keys_Typematic_Make_Break 0xFA
#define PS2_CMD_Set_All_Keys_Make 0xF9
#define PS2_CMD_Set_All_Keys_Make_Break 0xF8
#define PS2_CMD_Set_All_Keys_Typematic 0xF7
#define PS2_CMD_Set_Default 0xF6
#define PS2_CMD_Disable 0xF5
#define PS2_CMD_Enable 0xF4
#define PS2_CMD_Set_Typematic_Rate_Delay 0xF3
#define PS2_CMD_Read_ID 0xF2
#define PS2_CMD_Set_Scan_Code_Set 0xF0
#define PS2_CMD_Echo 0xEE
#define PS2_CMD_Set_Reset_LEDs 0xED

// Send default PS/2 responses
#define SEND_ACK() ringEnqueue(&sendBuffer, 0xFA)
#define SEND_ERROR() ringEnqueue(&sendBuffer, 0xFE)
#define SEND_BAT_OK() ringEnqueue(&sendBuffer, 0xAA)
#define SEND_ECHO() ringEnqueue(&sendBuffer, 0xEE)
#define SEND_ID() { ringEnqueue(&sendBuffer, 0xAB); ringEnqueue(&sendBuffer, 0x83); }

// Make and break code macros
#define MAKE_SPACE() ringEnqueue(&sendBuffer, 0x29)
#define MAKE_CODE(code) ringEnqueue(&sendBuffer, (code))

#define BREAK_SPACE() { ringEnqueue(&sendBuffer, 0xF0); ringEnqueue(&sendBuffer, 0x29); }
#define BREAK_CODE(code) { ringEnqueue(&sendBuffer, 0xF0); ringEnqueue(&sendBuffer, (code)); }

#endif
