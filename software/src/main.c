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


#define LED_1 (1<<PB0)
#define USB_LED_OFF 0
#define USB_LED_ON  1
#define USB_DATA_OUT 2

// testing eeprom read/write
static unsigned char write_data[16] = "eeprom_data_test";
static unsigned char debugData[16];
static unsigned char usbMsgData[16];

static unsigned char debugFlag = 0;

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

int main() {
    uchar i;
    DDRB = 1; // PB0 as output

    wdt_enable(WDTO_1S); // enable 1s watchdog timer

    // write some data to eeprom
    eeprom_update_block((const void *)write_data, (void *)0, 16);
    sprintf((char *)usbMsgData, "usbMsgData_test");

    usbInit();

    //turn on both LEDS
    PORTB |= LED_1;

    usbDeviceDisconnect(); // enforce re-enumeration
    for(i = 0; i<250; i++) { // wait 500 ms
        wdt_reset(); // keep the watchdog happy
        _delay_ms(2);
    }
    // turn off LED1
    PORTB &= ~LED_1;

    // read data from eeprom
    eeprom_read_block((void *)debugData, (const void *)0, 16);
    debugFlag = 1;

    usbDeviceConnect();

    sei(); // Enable interrupts after re-enumeration

    while(1) {
        wdt_reset(); // keep the watchdog happy
        usbPoll();
    }

    return 0;
}
