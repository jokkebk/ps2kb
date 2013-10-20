#include <avr/wdt.h> 
#include <avr/pgmspace.h>

#include "ps2config.h"
#include "ring.h"
#include "ps2.h"

#ifndef USE_BUTTON
#include "adc.h"
#endif

void sendCode(uint8_t code) {
    MAKE_CODE(code);
    for(millis = 0; millis < 10; ) 
        wdt_reset();
    BREAK_CODE(code);
}

#ifndef MINIMAL
// Not used currently, useful for debugging mainly
static prog_uint8_t hexCode[16] = {
    0x45, 0x16, 0x1E, 0x26, 
    0x25, 0x2E, 0x36, 0x3D,
    0x3E, 0x46, 0x1C, 0x32,
    0x21, 0x23, 0x24, 0x2B
};

void sendNibble(uint8_t n) {
    sendCode(pgm_read_byte(&hexCode[n]));
}

void sendHex(uint8_t hex) {
    sendNibble(hex >> 4);

    for(millis = 0; millis < 10; ) 
        wdt_reset();

    sendNibble(hex & 15);
}
#endif

int main(void) {
    uint16_t adc;
    uint8_t leds = 0, knocks = 0, state = 0;

    wdt_enable(WDTO_1S); // Enable watchdog timer to avoid hanging up

#ifdef USE_BUTTON
    BUTTON_PORT |= _BV(BUTTON_PIN); // pullup on button
#else
    adcStart();
#endif

    initPS2(); // Initializes timers also

#ifdef LED_PIN
    LED_DDR |= _BV(LED_PIN); // Initialize as output

    // Signal power-up with 3s LED
    LED_PORT |= _BV(LED_PIN); // ON
#endif

    // small delay after power-up to avoid sending random
    // stuff if power supply is fluctuating (possible?)
    for(millis = 0; millis < 3000; )
        wdt_reset(); 

#ifdef LED_PIN
    LED_PORT &= ~_BV(LED_PIN); // OFF
#endif

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

            // To make things simpler, we won't send space
            // presses while host has sent something or we
            // still have pending data in send buffer
            if(knocks >= 3 && ringEmpty(receiveBuffer) &&
                    ringEmpty(sendBuffer)) {
                sendCode(0x29);
                state = 1; // indicate we're sending
                knocks = 0;
            }

            millis = 0;
        }

        if(ps2Error) { // sending data was interrupted
            if(state == 1) { // we were sending space bar
                ringClear(&sendBuffer);
                BREAK_CODE(0x29); // send at least the break code
            } else {
                // Because send buffer is cleared when receiving
                // a PS/2 command, we can resend stuff just by
                // setting sendBuffer.read to "zero"
                sendBuffer.read = sendBuffer.buffer;
            }

            ps2Error = PS2ERROR_NONE; // resume business
        }

        if(state == 1 && ringEmpty(sendBuffer))
            state = 0; // seems like we succeeded!

        // Handle PS/2 commands - should be complete enough to fool
        // most PCs
        while(!ringEmpty(receiveBuffer)) {
            if(IS_PS2_CMD(*receiveBuffer.read))
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

                case PS2_CMD_Set_Reset_LEDs:
#ifndef MINIMAL 
                    // The "leds" variable is not currently used so
                    // don't store it in minimal version, just use
                    // same code as for typematic rate (ignore value)
                    SEND_ACK();
                    for(millis = 0; millis < 1000 && // wait 1s max
                            ringEmpty(receiveBuffer); )
                        wdt_reset();

                    if(!ringEmpty(receiveBuffer) && // received data
                            !IS_PS2_CMD(*receiveBuffer.read)) {
                        leds = ringDequeue(&receiveBuffer); // store
                        SEND_ACK();
                    } // else handle normally in next round
                    break;
#endif

                case PS2_CMD_Set_Typematic_Rate_Delay:
                    SEND_ACK();
                    for(millis = 0; millis < 1000 && // wait 1s max
                            ringEmpty(receiveBuffer); )
                        wdt_reset();

                    if(!ringEmpty(receiveBuffer) && // received data
                            !IS_PS2_CMD(*receiveBuffer.read)) {
                        ringDequeue(&receiveBuffer); // consume
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
