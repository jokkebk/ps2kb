#include <avr/io.h>
#include <avr/interrupt.h>

#include "ps2.h"
#include "timer.h"

// Free-running milliseconds counter
volatile uint16_t millis = 0;
volatile PS2Error ps2Error = PS2ERROR_NONE;

// PS/2 send and receive buffers
volatile RingBuffer sendBuffer, receiveBuffer;

void initPS2() {
	releaseClock();
	releaseData();

	ringClear(&sendBuffer); // clear ring
	ringClear(&receiveBuffer); // clear ring

	startTimer_0(50000L); // 50 kHz for clock generation
	startTimer_1(1000L); // 1 kHz for millisecond counting

    sei(); //  enable global interrupts
}

// 1000 Hz counter and clock logic
ISR(INT_VECT_1) { 
	millis++;
}

// PS/2 driver state machine starts here
static volatile uint8_t generateClock = 0;
static volatile uint8_t stateBits = 0, stateParity = 0, stateByte = 0;

typedef void *(*PS2Callback)();

void *cbIdle();
void *cbStillIdle();
void *cbInhibit();

void *cbSendBit();
void *cbSendParity();
void *cbSendStopBit();

void *cbReceiveBit();
void *cbReceiveParity();
void *cbReceiveAck();
void *cbReceiveEnd();

// 50 kHz, 20 us between calls
ISR(TIMER0_COMPA_vect) {
	static uint8_t clockPhase = 1;
	static PS2Callback cbCurrent = cbIdle;

	//sei(); // only callbacks take long and they take less than 4 calls

	if(clockPhase & 1) { // Middle of high/low
		if(clockPhase & 2) // High
			cbCurrent = (PS2Callback)(*cbCurrent)();
	} else if(generateClock) { // Transition
		// generateClock is only modified on clock high so additional
		// safeguards for clock left low shouldn't be necessary
		if(clockPhase & 2)
			releaseClock();
		else
			holdClock();
	} 

	clockPhase++;
}

// We should be idle (and not holding either data or clock line)
void *cbIdle() {
	generateClock = 0;

	if(isClockLow())
		return cbInhibit;

	return cbStillIdle;
}

// We were idle last time, and still are
void *cbStillIdle() {
	if(isClockLow())
		return cbInhibit;

	// ps2Error will hold data sending until cleared
	if(ps2Error || ringEmpty(sendBuffer)) 
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

// Send bit
void *cbSendBit() {
	if(isClockLow()) {
		releaseData(); // make sure data is released
		ps2Error = PS2ERROR_INTERRUPTED;
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
		releaseData(); // make sure data is released
		ps2Error = PS2ERROR_INTERRUPTED;
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
	if(isDataLow()) // data NOT released
		return cbReceiveAck; // generate clock pulses until released

	holdData();

	if(stateParity & 1) { // parity OK
		if(stateByte == 0xFE) // handle "resend" internally
			ringUnqueue(&sendBuffer); // resend last sent byte
		else // normal operation
			ringEnqueue(&receiveBuffer, stateByte); // store
	} else
		ringEnqueue(&receiveBuffer, PS2_Receive_Error);

	return cbReceiveEnd;
}

// End receiving
void *cbReceiveEnd() {
	releaseData();

	return cbIdle(); // avoid code duplication
}
