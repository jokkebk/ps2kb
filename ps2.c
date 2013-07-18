#include <avr/io.h>
#include <avr/interrupt.h>

#define F_CPU 20000000UL // 20 MHz

#include <util/delay.h>

#include "util.h"
#include "ring.h"

typedef enum {
	STATE_IDLE,
	STATE_INHIBIT,
	STATE_SENDING,
	STATE_RECEIVING
} ClockState;

// only two lowest bits are used: bit 1 indicates high/low
volatile uint8_t clockPhase = 0; // clock phase in bits 1:0 (1.1b)
volatile uint8_t busState = STATE_IDLE;
volatile uint8_t bitCount = 0, parity = 0, sendByte = 0;

volatile RingBuffer ring;

/*** MAIN METHOD ***/

#define BUTTON_DDR DDRD
#define BUTTON_PORT PORTD
#define BUTTON_PIN 6
#define BUTTON_INPUT PIND

int main(void) {
	BUTTON_PORT |= _BV(BUTTON_PIN); // pullup on button

	releaseClock();
	releaseData();

	ringClear(&ring); // clear ring
	
	TIMSK |= (1 << OCIE0A); // timer 0 (8-bit) overflow interrupt
	TCCR0A = (1 << WGM01); // WGM02:0 = 2, Clear Timer on Count
	TCCR0B = (1 << CS01); // timer 0 enable at clk/8

	// In CTC mode, TCNT0 goes up til OCR0A after which it is cleared
	OCR0A = 50 - 1; // At 20 MHz, this should result in 50 kHz

    sei(); //  enable global interrupts

	while(1) {
		if(!(BUTTON_INPUT & _BV(BUTTON_PIN))) {
			ringEnqueue(&ring, 0x29);
			_delay_ms(20);
			ringEnqueue(&ring, 0xF0);
			ringEnqueue(&ring, 0x29);
			_delay_ms(200);
		}
	}

	return 1;
}

/*** INTERRUPT HANDLER ***/

// Generate timer0 interrupts at 50 kHz (20us) to make 12.5 kHz clock
// About 150 cycles should be very safe for this routine
ISR(TIMER0_COMPA_vect) { // CTC OCIE0A interrupt
	static uint8_t idleCount = 0;

	if(busState == STATE_IDLE) {
		if(!isClockHigh()) { // Communication inhibited
			if(!isDataHigh()) { // PC wants to send
				busState = STATE_RECEIVING;
				bitCount = parity = clockPhase = 0;
			}
		} else if(idleCount < 3) { // idle for less than 60 us
			idleCount++;
		} else if(!ringEmpty(ring)) { // idle and data to send
			busState = STATE_SENDING;
			bitCount = parity = clockPhase = 0;
			sendByte = ringDequeue(&ring);
			holdData(); // start bit
			// note that clock will go low in 1/4 of wave now
		}
	} else if(busState == STATE_SENDING) {
		//if(bytesToSend()) {
		//	if(clockPhase & 2)
		//		releaseClock();
		//	else
		//		holdClock();
		//}
		//clockPhase++;
		if(clockPhase & 2) {
			if(clockPhase & 1) { // middle of high clock
				if(!isClockHigh()) { // PC has inhibited comms
					if(bitCount < 10)
						{} // TODO: Handle break
					busState == STATE_IDLE; // easiest this way
				} else if(bitCount < 8) {
					if(sendByte & 1) {
						releaseData();
						parity++;
					} else
						holdData();

					sendByte >>= 1;
				} else if(bitCount == 8) { // parity
					if(parity & 1)
						holdData();
					else
						releaseData();
				} else if(bitCount == 9) { // stop
					releaseData();
				} else { // end
					busState = STATE_IDLE;
				}
				bitCount++;
			} else { // start of high clock
				releaseClock(); // get clock high
			}
		} else {
			if(!(clockPhase & 1)) // start of low clock
				holdClock();
		}

		clockPhase++; // next clock phase
	} else { // STATE_RECEIVING
		// TODO
	}
}

