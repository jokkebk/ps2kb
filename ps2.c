#include <stdio.h> // NULL

#include <avr/io.h>
#include <avr/interrupt.h>

#define F_CPU 20000000UL // 20 MHz

#define BUTTON_PORT PORTD
#define BUTTON_PIN 6
#define BUTTON_DOWN() (!(PIND & _BV(BUTTON_PIN)))

#include <util/delay.h>

#include "util.h"
#include "ring.h"

// Free-running milliseconds counter
static volatile uint16_t millis = 0;

static volatile uint8_t generateClock = 0;
static volatile uint8_t stateBits = 0, stateParity = 0, stateByte = 0;

static volatile RingBuffer sendBuffer, receiveBuffer;

typedef void *(*PS2Callback)();

static volatile PS2Callback cbCurrent = NULL;

void *cbIdle();
void *cbStillIdle();
void *cbInhibit();

void *cbSendStartBit();
void *cbSendBit();
void *cbSendParity();
void *cbSendStopBit();

void *cbReceiveBit();
void *cbReceiveParity();
void *cbReceiveAck();
void *cbReceiveEnd();

int main(void) {
	uint8_t buttonDown = 0, repeat = 0, lastByte = 0;

	BUTTON_PORT |= _BV(BUTTON_PIN); // pullup on button

	releaseClock();
	releaseData();

	ringClear(&sendBuffer); // clear ring
	ringClear(&receiveBuffer); // clear ring

	cbCurrent = cbIdle; // start state
	
	// Init timer1 (1 kHz clock and logic)
	TCCR1B |= (1 << WGM12); // configure for CTC mode
	TIMSK |= (1 << OCIE1A); // CTC interrupt
	OCR1A = 1250 - 2; // with prescaler 8, results in 1000 Hz @ 20 MHz
	TCCR1B |= (1 << CS11); // timer 1 at clk/8

	// Init timer0 (50 kHz clock generation / logic)
	TIMSK |= (1 << OCIE0A); // timer 0 (8-bit) output compare interrupt
	TCCR0A = (1 << WGM01); // WGM02:0 = 2, Clear Timer on Count
	OCR0A = 50 - 1; // At 20 MHz, this should result in 50 kHz
	TCCR0B = (1 << CS01); // timer 0 enable at clk/8

    sei(); //  enable global interrupts

	while(1) {
		// Handle button press
		if(BUTTON_DOWN()) { // mechanical button down
			if(!buttonDown) { // keypress
				buttonDown = 1;
				ringEnqueue(&sendBuffer, lastByte = 0x29);
				millis = 0;
				repeat = 0;
			} else if(millis > 500 && !repeat || 
					(millis > 333 && repeat)) {
				ringEnqueue(&sendBuffer, lastByte = 0x29);
				repeat = 1; // inconsequental if repeat already on
				millis = 0;
			}
		} else if(buttonDown && millis > 10) { // debounced key release
			buttonDown = 0;
			ringEnqueue(&sendBuffer, 0xF0);
			ringEnqueue(&sendBuffer, lastByte = 0x29);
		}

		// Handle PS/2 commands - should be complete enough to fool
		// most PCs
		if(!ringEmpty(receiveBuffer)) {
			switch(ringDequeue(&receiveBuffer)) {
				case 0: // unknown/invalid command
					ringEnqueue(&sendBuffer, 0xFE);
					break;

				case 0xEE: // echo
					ringEnqueue(&sendBuffer, lastByte = 0xEE);
					break;

				case 0xFF: // reset
					ringEnqueue(&sendBuffer, lastByte = 0xFA);
					// will break repeat but so what
					for(millis = 0; millis < 10; ) {}
					ringEnqueue(&sendBuffer, lastByte = 0xAA);
					break;

				case 0xFE: // resend
					// This doesn't actually work for multi-byte sends!
					ringEnqueue(&sendBuffer, lastByte);
					break;

				case 0xF2: // read ID
					ringEnqueue(&sendBuffer, 0xAB);
					ringEnqueue(&sendBuffer, lastByte = 0x83);
					break;

				case 0xED: case 0xF3: // Set LEDS / typematic rate
					// Both can result in host sending 0x00 next so
					// let's consume the parameter now

					ringEnqueue(&sendBuffer, lastByte = 0xFA);

					while(ringEmpty(receiveBuffer)) {} // wait...

					if(ringHead(receiveBuffer) < 0xED)
						ringDequeue(&receiveBuffer); // consume

					ringEnqueue(&sendBuffer, lastByte = 0xFA);
					break;

				default: // 0xED, etc.
					// Most other commands can be adequately emulated
					// with just acknowledging everything, including
					// set typematic etc. rates.
					//
					// Current implementation does not stop sending
					// key presses while executing these commands,
					// which is a shame.
					ringEnqueue(&sendBuffer, lastByte = 0xFA);
					break;
			}
		}
	}

	return 1;
}

// 1000 Hz counter and clock logic
ISR(TIMER1_COMPA_vect) { 
	millis++;
}

// We should be idle
void *cbIdle() {
	generateClock = 0;

	if(isClockLow())
		return cbInhibit;

	return cbStillIdle;
}

// We were idle last time
void *cbStillIdle() {
	if(isClockLow())
		return cbInhibit;

	if(ringEmpty(sendBuffer)) // no data to send
		return cbStillIdle;

	holdData(); // Start bit (0)
	generateClock = 1;

	stateByte = ringDequeue(&sendBuffer);
	stateParity = 0;
	stateBits = 8;

	return cbSendBit;
}

// Clock line was held low last time
void *cbInhibit() {
	if(isClockLow()) // still held low
		return cbInhibit;

	if(isDataHigh()) // no request to send
		return cbIdle;

	stateParity = 0;
	stateBits = 8;

	generateClock = 1;

	return cbReceiveBit;
}

// Send start bit
void *cbSendStartBit() {
	if(isClockLow()) {
		return cbInhibit; // no need to retransmit data
	}

	holdData(); // 0

	stateByte = ringDequeue(&sendBuffer);
	stateParity = 0;
	stateBits = 8;
	generateClock = 1;

	return cbSendBit;
}

// Send bit
void *cbSendBit() {
	if(isClockLow()) {
		// TODO: Signal main program that send was aborted
		return cbInhibit;
	}

	if(stateByte & 1) {
		stateParity++;
		releaseData();
	} else
		holdData();

	stateByte >>= 1;
	
	if(--stateBits)
		return cbSendBit;

	return cbSendParity;
}

// Send parity bit
void *cbSendParity() {
	if(isClockLow()) {
		// TODO: Signal main program that send was aborted
		return cbInhibit;
	}

	if(stateParity & 1) {
		holdData(); // send zero for odd parity
	} else
		releaseData();

	return cbSendStopBit;
}

// Send stop bit
void *cbSendStopBit() {
	// No need to worry about clock being held low anymore

	releaseData(); // Just release data (1)

	return cbIdle;
}

// Receive one bit
void *cbReceiveBit() {
	stateByte >>= 1;

	if(isDataHigh()) {
		stateByte |= 0x80;
		stateParity++;
	}

	if(--stateBits)
		return cbReceiveBit;

	return cbReceiveParity;
}

// Receive parity
void *cbReceiveParity() {
	if(isDataHigh())
		stateParity++;

	return cbReceiveAck;
}

// Send ACK 
void *cbReceiveAck() {
	if(isDataLow()) { // data NOT released
		// TODO: Generate error
		return cbReceiveAck; // generate clock pulses until released
	}

	holdData();

	if(stateParity & 1) // parity OK
		ringEnqueue(&receiveBuffer, stateByte); // store
	else
		ringEnqueue(&receiveBuffer, 0); // indicate error

	return cbReceiveEnd;
}

// End receiving
void *cbReceiveEnd() {
	releaseData();

	return cbIdle(); // avoid code duplication
}

// Supposed clock phase is stored in clockPhase with 2 lowest bits
// defining the state, and numbered thus:
//   0 = high to low transition
//   1 = middle low
//   2 = low to high transition
//   3 = middle high

// 50 kHz, 20 us between calls
ISR(TIMER0_COMPA_vect) { // CTC OCIE0A interrupt
	static uint8_t clockPhase;

	sei(); // only callbacks take long and they take less than 4 calls

	if(clockPhase & 1) { // Middle of high/low
		if(clockPhase & 2) // High
			cbCurrent = (PS2Callback)(*cbCurrent)();
	} else if(generateClock) { // Transition
		if(clockPhase & 2)
			releaseClock();
		else
			holdClock();
	} 

	clockPhase++;
}

