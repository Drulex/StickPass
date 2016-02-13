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

// To keep track of number of credentials in EEPROM
unsigned char credCount;

// To iterate through debugData when building interrupt_in messages
volatile unsigned char msgPtr;

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

    uchar i;

    // Variables initialization
    msgPtr = 0;
    credCount = 0;

    memset(usbMsgData, 0, 8);
    memset(debugData, 0, 64);
    snprintf((char *)usbMsgData, 8, "usbMsg");
    snprintf((char *)debugData, 64, "passwordTestData1234567890*&^$!#");
    debugFlag = 1;

/*
    // some test data
    char idBlockTest[ID_BLOCK_LEN] = "testappn\0\0test12345.jora@gmail.com\0\0\0\0\0\0\0\0password123\0\0\0\0\0\0\0\0\0\0\0";
    cred_t cred;
    parseIdBlock(&cred, idBlockTest);
    update_credential(cred);

    //eeprom_read_block((void *)debugData, (const void *)10, 32);
*/

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
            uchar sendKey = debugData[msgPtr];
            switch(state) {
                case STATE_SEND_KEY:
                    buildReport(sendKey);
                    state = STATE_RELEASE_KEY;
                    //msgPtr++;
                    break;

                case STATE_RELEASE_KEY:
                    buildReport(0);
                    msgPtr++;
                    if(debugData[msgPtr] == '\0') {
                        flagDone = 1;
                        state = STATE_WAIT;
                        msgPtr = 0;
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
