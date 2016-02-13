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

unsigned char pbCounter = 0;

typedef struct {
        uint8_t modifier;
        uint8_t reserved;
        uint8_t keycode[6];
} keyboard_report_t;

static keyboard_report_t keyboard_report; // sent to PC
static uchar idleRate; // repeat rate for keyboards

usbMsgLen_t usbFunctionSetup(uchar data[8]) {
    usbRequest_t *rq = (void *)data;

    if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) {
        switch(rq->bRequest) {

            //send NO_KEYS_PRESSED
            case USBRQ_HID_GET_REPORT:
                usbMsgPtr = (void *)&keyboard_report;
                keyboard_report.modifier = 0;
                keyboard_report.keycode[0] = 0;

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

    // pushbutton init
    PORTB = (1<<PB3); // PB1 is input with internal pullup resistor activated

    uchar i;

    // Variables initialization
    msgPtr = 0;
    credCount = 0;

    memset(usbMsgData, 0, 8);
    memset(debugData, 0, 64);
    snprintf((char *)usbMsgData, 8, "usbMsg");
    snprintf((char *)debugData, 64, "this_is_a_interrupt_in_test");
    debugFlag = 1;

    // some test data
    char idBlockTest[ID_BLOCK_LEN] = "testappn\0\0test12345.jora@gmail.com\0\0\0\0\0\0\0\0password123\0\0\0\0\0\0\0\0\0\0\0";
    cred_t cred;
    parseIdBlock(&cred, idBlockTest);
    update_credential(cred);

    //eeprom_read_block((void *)debugData, (const void *)10, 32);

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
        if(usbInterruptIsReady() && debugFlag) {
            unsigned char msg[8];
            uchar len = getInterruptData(msg);
            if(len > 0)
                usbSetInterrupt(msg, len);
        }

        if(!(PINB & (1<<PB3))) {
            if(pbCounter == 255) {
                memset(debugData, 0, 64);
                snprintf((char *)debugData, 64, "pushbutton was pressed");
                debugFlag = 1;
                LED_TOGGLE();
            }
            pbCounter = 0;
        }
        // debouncing
        if(pbCounter < 255)
            pbCounter++;
    }
    return 0;
}
