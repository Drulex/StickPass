/*
 * TODO
 * ====
 *
 *  COMMUNICATIONS
 *  --------------
 *    Routine to send debug data to user app on a constant basis
 *    Routine to parse incoming ID_BLOCK (64 bytes containing id_name, username, password)
 *
 *  MEMORY MANAGEMENT
 *  -----------------
 *    Routine to update EEPROM memory with parsed elements from ID_BLOCK
 *    Memory encryption and decryption
 *
 *  HID FIRMWARE
 *    Implementation of keyboard usb_descriptor for HID device
 *
 *
 */

#include <avr/io.h>
#include <stdio.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>

// v-usb library
#include "usbdrv.h"
#include "credentials.h"


#define LED_1 (1<<PB0)
#define USB_LED_OFF 0
#define USB_LED_ON  1
#define USB_DATA_OUT 2

// testing eeprom read/write
static unsigned char debugData[32];
static unsigned char usbMsgData[32];

static unsigned char debugFlag = 0;

unsigned char credCount;
unsigned char msgPtr;

// this gets called when custom control message is received
USB_PUBLIC uchar usbFunctionSetup(uchar data[8]) {
    usbRequest_t *rq = (void *)data; // cast data to correct type

    switch(rq->bRequest) { // custom command is in the bRequest field
        case USB_LED_ON:
            PORTB |= LED_1; // turn LED on
            return 0;

        case USB_LED_OFF:
            PORTB &= ~LED_1; // turn LED off
            return 0;

        case USB_DATA_OUT:
            if(debugFlag) {
                usbMsgPtr = debugData;
                debugFlag = 0;
                return sizeof(debugData);
            }
            else {
                usbMsgPtr = usbMsgData;
                return sizeof(usbMsgData);
            }
    }

    return 0; // should not get here
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
    // should be 8
    return i;
}

int main() {
    uchar i;
    msgPtr = 0;
    DDRB = 1; // PB0 as output

    //turn on LED
    PORTB |= LED_1;

    // some test data
    char idBlockTest[ID_BLOCK_LEN] = "testappn\0\0test12345.jora@gmail.com\0\0\0\0\0\0\0\0password123\0\0\0\0\0\0\0\0\0\0\0";
    cred_t cred;
    parseIdBlock(&cred, idBlockTest);
    update_credential(cred);

    // for testing this should be in eeprom eventually
    credCount = 0;

    sprintf((char *)usbMsgData, "usbMsgData_test");
    sprintf((char *)debugData, "interrupt_in_test");

    debugFlag = 1;
    //eeprom_read_block((void *)debugData, (const void *)10, 32);

    usbInit();

    usbDeviceDisconnect(); // enforce re-enumeration
    for(i = 0; i<250; i++) { // wait 500 ms
        wdt_reset(); // keep the watchdog happy
        _delay_ms(2);
    }
    // turn off LED1
    PORTB &= ~LED_1;

    usbDeviceConnect();

    wdt_enable(WDTO_1S); // enable 1s watchdog timer
    sei(); // Enable interrupts after re-enumeration

    while(1) {
        wdt_reset(); // keep the watchdog happy
        usbPoll();
        if(usbInterruptIsReady() && debugFlag) {
            PORTB |= LED_1;
            unsigned char msg[8];
            uchar len = getInterruptData(msg);
            if(len > 0)
                usbSetInterrupt(msg, len);
        }
    }

    return 0;
}
