#include <avr/wdt.h> 
#include <avr/pgmspace.h>

#include "ps2config.h"
#include "ring.h"
#include "ps2.h"

#ifndef USE_BUTTON
#include "adc.h"
#endif

static prog_uint8_t hexCode[16] = {
	0x45, 0x16, 0x1E, 0x26, 
	0x25, 0x2E, 0x36, 0x3D,
	0x3E, 0x46, 0x1C, 0x32,
	0x21, 0x23, 0x24, 0x2B
};

void sendCode(uint8_t code) {
	MAKE_CODE(code);
	for(millis = 0; millis < 10; ) 
		wdt_reset();
	BREAK_CODE(code);
}

void sendNibble(uint8_t n) {
	sendCode(pgm_read_byte(&hexCode[n]));
}

void sendHex(uint8_t hex) {
	sendNibble(hex >> 4);
		
	for(millis = 0; millis < 10; ) 
		wdt_reset();

	sendNibble(hex & 15);
}

int main(void) {
	uint8_t buttonDown = 0, repeat = 0, leds = 0;
	uint16_t adc;
	uint8_t knocks = 0;

	wdt_enable(WDTO_1S); // Enable watchdog timer to avoid hanging up

#ifdef USE_BUTTON
	BUTTON_PORT |= _BV(BUTTON_PIN); // pullup on button
#else
	adcStart();
#endif

#ifdef LED_PIN // Have a LED
	LED_DDR |= _BV(LED_PIN); // Initialize as output
#endif

	initPS2(); // Initializes timers also

	// Signal power-up with 3s LED
	LED_PORT |= _BV(LED_PIN); // ON

	for(millis = 0; millis < 3000; )
		wdt_reset();

	LED_PORT &= ~_BV(LED_PIN); // OFF

	while(1) {
		wdt_reset(); // reset watchdog

#ifdef USE_BUTTON
		if(BUTTON_DOWN() && millis > 500) {
#else // ADC
		if((adc = adcRead()) > ADC_TRESHOLD && millis > 500) {
#endif
			if(millis > 3000)
				knocks = 1;
			else
				knocks++;

			if(knocks >= 3) {
				sendCode(0x29);
				knocks = 0;
			}

			millis = 0;
		}

		// Handle PS/2 commands - should be complete enough to fool
		// most PCs
		while(!ringEmpty(receiveBuffer)) {
			if(IS_PS2_CMD(ringHead(receiveBuffer)))
				ringClear(&sendBuffer); // clear send buffer on command

			switch(ringDequeue(&receiveBuffer)) {
				case PS2_Receive_Error: // unknown/invalid command
					SEND_ERROR();
					break;

				// Resend (0xFE) is handled by PS2 code internally

				case PS2_CMD_Echo:
					SEND_ECHO();
					break;

				case PS2_CMD_Reset:
					SEND_ACK();
					// will break repeat but so what
					for(millis = 0; millis < 10; ) 
						wdt_reset();
					SEND_BAT_OK();
					break;

				case PS2_CMD_Read_ID:
					SEND_ID();
					break;

				case PS2_CMD_Set_Typematic_Rate_Delay:
					SEND_ACK();
					for(millis = 0; millis < 1000 && // wait 1s max
							ringEmpty(receiveBuffer); )
						wdt_reset();

					if(!ringEmpty(receiveBuffer) && // received data
							!IS_PS2_CMD(ringHead(receiveBuffer))) {
						ringDequeue(&receiveBuffer); // consume
						SEND_ACK();
					} // else handle normally in next round
					break;

				case 0xED:
					SEND_ACK();
					for(millis = 0; millis < 1000 && // wait 1s max
							ringEmpty(receiveBuffer); )
						wdt_reset();

					if(!ringEmpty(receiveBuffer) && // received data
							!IS_PS2_CMD(ringHead(receiveBuffer))) {
						leds = ringDequeue(&receiveBuffer); // store
						SEND_ACK();
					} // else handle normally in next round
					break;

				default:
					// Most other commands can be adequately emulated
					// with just acknowledging everything, including
					// set typematic etc. rates.
					SEND_ACK();
					break;
			}
		}
	}

	return 1;
}
