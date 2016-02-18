/*
 * File: main.c
 * Project: StickPass
 * Author: Alexandru Jora (alexandru@jora.ca)
 * Creation Date: 2016-01-27
 * License: GNU GPL v2 (see License.txt)
 *
 */

#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>

#include "usbdrv.h"
#include "credentials.h"
#include "led.h"
#include "hid.h"

#define NUM_LOCK 1
#define CAPS_LOCK 2
#define SCROLL_LOCK 4

#define STATE_WAIT 0
#define STATE_SEND_KEY 1
#define STATE_RELEASE_KEY 2

// Buffer to send debug messages on interrupt endpoint 1
unsigned char debugData[64];

// Buffer to reply to custom control messages on endpoint 0
unsigned char usbMsgData[8];

// Debug flag. Set this to 1 after filling debugData buffer to send it
volatile unsigned char debugFlag = 0;

// To iterate through debugData when building interrupt_in messages
volatile unsigned char msgPtr;

static unsigned char capsCounter = 0;

// init
unsigned char pbCounter = 0;
unsigned char state = STATE_WAIT;
unsigned char flagDone = 0;

keyboard_report_t keyboard_report;
volatile static uchar LED_state = 0xff;
static unsigned char idleRate;

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

usbMsgLen_t usbFunctionSetup(uchar data[8]) {
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

usbMsgLen_t usbFunctionWrite(uint8_t * data, uchar len) {
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

uchar getInterruptData(uchar *msg) {
    uchar i;
    for(i = 0; i < 8; i++) {
        if(debugData[msgPtr] == '\0') {
            msgPtr = 0;
            debugFlag = 0;
            return i;
        }
        msg[i] = debugData[msgPtr];
        msgPtr++;
    }
    return i;
}

int main() {
    // Modules initialization
    LED_Init();
    PB_INIT();
    clearKeyboardReport();

    char idBlockTest1[ID_BLOCK_LEN] = "linkedin\0\0alexandru.jora@gmail.com\0\0\0\0\0\0\0\0password123\0\0\0\0\0\0\0\0\0\0\0";
    char idBlockTest2[ID_BLOCK_LEN] = "gmail\0\0\0\0\0test12345.test@gmail.com\0\0\0\0\0\0\0\0p23sword123456789\0\0\0\0\0";
    char idBlockTest3[ID_BLOCK_LEN] = "somebank\0\0bank54645usert@babnk.com\0\0\0\0\0\0\0\0ftji8o!@#$3456789\0\0\0\0\0";

    generateCredentialsTestData(idBlockTest1);
    _delay_ms(200);
    generateCredentialsTestData(idBlockTest2);
    _delay_ms(200);
    generateCredentialsTestData(idBlockTest3);

    uchar i;

    credPtr = 0;

    memset(debugData, 0, 64);

    getCredentialData(1, &debugData[0]);

    cli();

    LED_HIGH();
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

    if(credCount == 2)
        LED_HIGH();

    while(1) {
        wdt_reset();
        usbPoll();
        if(!(PINB & (1<<PB3))) {
            if(state == STATE_WAIT && pbCounter == 255) {
                // proper send request
                state = STATE_SEND_KEY;
                flagDone = 0;
            }
            pbCounter = 0;
        }
        // debouncing
        if(pbCounter < 255)
            pbCounter++;

        if(usbInterruptIsReady() && state != STATE_WAIT && !flagDone) {
            uchar sendKey = debugData[credPtr];
            switch(state) {
                case STATE_SEND_KEY:
                    buildReport(sendKey);
                    state = STATE_RELEASE_KEY;
                    break;

                case STATE_RELEASE_KEY:
                    buildReport(0);
                    credPtr++;
                    if(debugData[credPtr] == '\0') {
                        flagDone = 1;
                        state = STATE_WAIT;
                        updateCredPtr();
                    }

                    else {
                        state = STATE_SEND_KEY;
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
