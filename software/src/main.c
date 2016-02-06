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

// Buffer to send debug messages on interrupt endpoint 1
volatile unsigned char debugData[64];

// Buffer to reply to custom control messages on endpoint 0
volatile unsigned char usbMsgData[8];

// Debug flag. Set this to 1 after filling debugData buffer to send it
volatile unsigned char debugFlag = 0;

// To keep track of number of credentials in EEPROM
unsigned char credCount;

// To iterate through debugData when building interrupt_in messages
volatile unsigned char msgPtr;

// this gets called when custom control message is received
USB_PUBLIC uchar usbFunctionSetup(uchar data[8]) {
    usbRequest_t *rq = (void *)data;
    switch(rq->bRequest) {
        case USB_LED_ON:
            LED_HIGH();
            return 0;

        case USB_LED_OFF:
            LED_LOW();
            return 0;

        case USB_DATA_OUT:
            usbMsgPtr = usbMsgData;
            return sizeof(usbMsgData);
    }
    return 0;
}

uchar getInterruptData(uchar *msg) {
    uchar i;
    for(i = 0; i < 8; i++) {
        if(debugData[msgPtr] == '\0') {
            msgPtr = 0;
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

    uchar i;

    // Variables initialization
    msgPtr = 0;
    credCount = 0;

    snprintf((char *)usbMsgData, 8, "usbMsg");
    snprintf((char *)debugData, 64, "this_is_a_interrupt_in_test");
    debugFlag = 1;

    // some test data
    char idBlockTest[ID_BLOCK_LEN] = "testappn\0\0test12345.jora@gmail.com\0\0\0\0\0\0\0\0password123\0\0\0\0\0\0\0\0\0\0\0";
    cred_t cred;
    parseIdBlock(&cred, idBlockTest);
    update_credetial(cred);

    //eeprom_read_block((void *)debugData, (const void *)10, 32);

    LED_HIGH();
    usbInit();

    usbDeviceDisconnect(); // enforce re-enumeration
    for(i = 0; i<250; i++) { // wait 500 ms
        wdt_reset(); // keep the watchdog happy
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
            LED_TOGGLE();
            unsigned char msg[8];
            uchar len = getInterruptData(msg);
            if(len > 0)
                usbSetInterrupt(msg, len);
        }
    }
    return 0;
}
