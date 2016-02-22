/*
 * File: main.c
 * Project: StickPass
 * Author: Alexandru Jora (alexandru@jora.ca)
 * Creation Date: 2016-01-27
 * License: GNU GPL v2 (see License.txt)
 *
 */

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>

#include "usbdrv.h"
#include "credentials.h"
#include "led.h"
#include "hid.h"
#include "timer1.h"

#define NUM_LOCK 1
#define CAPS_LOCK 2
#define SCROLL_LOCK 4

#define STATE_WAIT 0
#define STATE_SHORT_KEY 1
#define STATE_LONG_KEY 2
#define STATE_RELEASE_KEY 3

// Buffer to send debug messages on interrupt endpoint 1
static unsigned char debugData[64];

// init
static unsigned char pbCounter = 0;
static unsigned char state = STATE_WAIT;
static unsigned char flagDone = 0;

keyboard_report_t keyboard_report;
volatile static unsigned char LED_state = 0xff;
static unsigned char idleRate;

// for PB long press detection
static unsigned char pbHold = 0;

// hid descriptor stored in flash
const PROGMEM char usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x08,                    //   REPORT_COUNT (8)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)(Key Codes)
    0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard LeftControl)(224)
    0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)(231)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs) ; Modifier byte
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x81, 0x03,                    //   INPUT (Cnst,Var,Abs) ; Reserved byte
    0x95, 0x05,                    //   REPORT_COUNT (5)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x05, 0x08,                    //   USAGE_PAGE (LEDs)
    0x19, 0x01,                    //   USAGE_MINIMUM (Num Lock)
    0x29, 0x05,                    //   USAGE_MAXIMUM (Kana)
    0x91, 0x02,                    //   OUTPUT (Data,Var,Abs) ; LED report
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x03,                    //   REPORT_SIZE (3)
    0x91, 0x03,                    //   OUTPUT (Cnst,Var,Abs) ; LED report padding
    0x95, 0x06,                    //   REPORT_COUNT (6)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x65,                    //   LOGICAL_MAXIMUM (101)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)(Key Codes)
    0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))(0)
    0x29, 0x65,                    //   USAGE_MAXIMUM (Keyboard Application)(101)
    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
    0xc0                           // END_COLLECTION
};

usbMsgLen_t usbFunctionSetup(unsigned char data[8]) {
    usbRequest_t *rq = (void *)data;

    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) {
        switch(rq->bRequest) {

            //send NO_KEYS_PRESSED
            case USBRQ_HID_GET_REPORT:
                usbMsgPtr = (void *)&keyboard_report;
                keyboard_report.modifier = 0;
                keyboard_report.keycode = 0;

                return sizeof(keyboard_report);

            case USBRQ_HID_SET_REPORT: // if wLength == 1, should be LED state
                return (rq->wLength.word == 1) ? USB_NO_MSG : 0;

            // send idleRate to PC as per spec
            case USBRQ_HID_GET_IDLE: // send idle rate to PC as required by spec
                usbMsgPtr = &idleRate;
                return 1;

            // set idleRate as per spec
            case USBRQ_HID_SET_IDLE: // save idle rate as required by spec
                idleRate = rq->wValue.bytes[1];
                return 0;
        }
    }
    return 0;
}

usbMsgLen_t usbFunctionWrite(uint8_t * data, unsigned char len) {
    if (data[0] == LED_state)
        return 1;
    else
        LED_state = data[0];

    // LED state changed
    if(LED_state & CAPS_LOCK)
        LED_HIGH();
    else
        LED_LOW();

    return 1;
}

int main() {
    unsigned char i;
    char idBlockTest1[ID_BLOCK_LEN] = "linkedin\0\0alexandru.jora@gmail.com\0\0\0\0\0\0\0\0password123\0\0\0\0\0\0\0\0\0\0\0";

    // Modules initialization
    LED_Init();
    timer1_Init();
    PB_INIT();
    LED_HIGH();

    // debug
    clearKeyboardReport();
    generateCredentialsTestData(idBlockTest1);

    // var init
    credPtr = 0;
    memset(debugData, 0, 64);
    getCredentialData(0, &debugData[0]);

    cli();

    usbInit();

    // enforce re-enumeration
    usbDeviceDisconnect();
    for(i = 0; i<250; i++) {
        wdt_reset();
        _delay_ms(2);
    }

    usbDeviceConnect();

    LED_LOW();

    // Enable 1s watchdog
    wdt_enable(WDTO_1S);

    // Enable global interrupts after re-enumeration
    sei();

    while(1) {
        wdt_reset();
        usbPoll();

        // PB press detection
        if(!(PINB & (1<<PB3))) {
            // debouncing
            if(state == STATE_WAIT && pbCounter == 255) {
                // reset counter
                counter100ms = 0;
                while(!(PINB & (1<<PB3)) && !pbHold) {
                    // waiting for PB long press event
                    wdt_reset();
                    // if PB is held for 1.5s
                    if(counter100ms >= 15) {
                        state = STATE_LONG_KEY;
                        // send password
                        credPtr = 42;
                        pbHold = 1;
                        flagDone = 0;
                    }
                }
                // short PB press
                state = STATE_SHORT_KEY;
                flagDone = 0;
            }
            pbHold = 0;
            pbCounter = 0;
        }
        // debouncing
        if(pbCounter < 255)
            pbCounter++;

        if(usbInterruptIsReady() && state != STATE_WAIT && !flagDone) {
            unsigned char sendKey = debugData[credPtr];
            switch(state) {
                case STATE_SHORT_KEY:
                    buildReport(sendKey);
                    state = STATE_RELEASE_KEY;
                    break;

                case STATE_LONG_KEY:
                    buildReport(sendKey);
                    state = STATE_RELEASE_KEY;

                case STATE_RELEASE_KEY:
                    buildReport(0);
                    credPtr++;
                    if(debugData[credPtr] == '\0') {
                        flagDone = 1;
                        state = STATE_WAIT;
                        credPtr = 0;
                    }

                    else {
                        state = STATE_SHORT_KEY;
                    }

                    break;

                default:
                    state = STATE_WAIT; // should not happen
            }

            usbSetInterrupt((void *)&keyboard_report, sizeof(keyboard_report));
            LED_TOGGLE();
        }
    }
    return 0;
}
