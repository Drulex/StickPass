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


#define USB_LED_OFF 0
#define USB_LED_ON 1
#define USB_DATA_OUT 2

#define STATE_ID_UPLOAD 3
#define STATE_ID_UPLOAD_INIT 4
#define STATE_ID_NAME_SEND 5
#define STATE_ID_NAME_DONE 6
#define STATE_ID_USERNAME_SEND 7
#define STATE_ID_USERNAME_DONE 8
#define STATE_ID_PASS_SEND 9
#define STATE_ID_PASS_DONE 10
#define STATE_ID_UPLOAD_DONE 11


// Buffer to send debug messages on interrupt endpoint 1
static unsigned char debugData[64 + 1];

// init
static unsigned char pbCounter = 0;
static unsigned char state = STATE_WAIT;
static unsigned char flagDone = 0;
static unsigned char flagCredReady = 0;
static unsigned char idMsgPtr = 0;
static unsigned char idState;

keyboard_report_t keyboard_report;
cred_t cred;

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
    // other requests
    else {
        switch(rq->bRequest) {
            case USB_LED_ON:
                LED_HIGH();
                return 0;

            case USB_LED_OFF:
                LED_LOW();
                return 0;

            case STATE_ID_UPLOAD:
                return USB_NO_MSG;
        }
    }
    return 0;
}


usbMsgLen_t usbFunctionWrite(uint8_t * data, unsigned char len) {
    unsigned char i;
    idState = data[0];
    switch(idState) {
            case STATE_ID_UPLOAD_INIT:
                // clear credentials structure and reset msg pointer
                clearCred();
                idMsgPtr = 0;
                return 1;

            case STATE_ID_NAME_SEND:
                for(i = 1; i < len; i++) {
                    cred.idName[idMsgPtr] = data[i];
                    idMsgPtr++;
                }
                return 1;

            case STATE_ID_NAME_DONE:
                //cred.idName[idMsgPtr] = '\0';
                idMsgPtr = 0;
                return 1;

            case STATE_ID_USERNAME_SEND:
                for(i = 1; i < len; i++) {
                    cred.idUsername[idMsgPtr] = data[i];
                    idMsgPtr++;
                }
                return 1;

            case STATE_ID_USERNAME_DONE:
                //cred.idUsername[idMsgPtr] = '\0';
                idMsgPtr = 0;
                return 1;

            case STATE_ID_PASS_SEND:
                for(i = 1; i < len; i++) {
                    cred.idPassword[idMsgPtr] = data[i];
                    idMsgPtr++;
                }
                return 1;

            case STATE_ID_PASS_DONE:
                //cred.idPassword[idMsgPtr] = '\0';
                //idMsgPtr++;
                flagCredReady = 1;
                LED_TOGGLE();
                //update_credential();
                return 1;
    }

    return 1;
}

int main() {
    unsigned char i, j;

    // Modules initialization
    LED_Init();
    timer1_Init();
    PB_INIT();
    LED_HIGH();

    // debug
    clearKeyboardReport();
    //generateCredentialsTestData(idBlockTest1);

    // var init
    credPtr = 0;
    memset(debugData, 0, 64 + 1);
    //getCredentialData(0, &debugData[0]);

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
        if(flagCredReady) {
            LED_TOGGLE();
            j = 0;
            for(i = 0; i < 10; i++) {
                debugData[j] = cred.idName[i];
                j++;
            }
            for(i = 0; i < 32; i++) {
                debugData[j] = cred.idUsername[i];
                j++;
            }
            for(i = 0; i < 22; i++) {
                debugData[j] = cred.idPassword[i];
                j++;
            }
            LED_TOGGLE();
            flagCredReady = 0;
        }

        // PB press detection
        if(!(PINB & (1<<PB3))) {
            // debouncing
            if(state == STATE_WAIT && pbCounter == 255) {
                // reset counter
                counter100ms = 0;
                while(!(PINB & (1<<PB3)) && !pbHold) {
                    // waiting for PB long press event
                    wdt_reset();
                    // if PB is held for 1.0s
                    if(counter100ms >= 10) {
                        counter100ms = 0;
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
                        //credPtr = 0;
                        updateCredPtr();
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
