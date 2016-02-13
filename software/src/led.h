/*
 * File: led.h
 * Project: StickPass
 * Author: Alexandru Jora (alexandru@jora.ca)
 * Creation Date: 2016-01-27
 * License: GNU GPL v2 (see License.txt)
 *
 * Some general purpose macros to help using LED
 */

#ifndef LED_H
#define LED_H

// Using LED on PB4
#define LED_DDR         (DDRB)
#define LED_PORT        (PORTB)
#define LED             (4)

#define LED_Init()      LED_OUTPUT()
#define LED_OUTPUT()    (LED_DDR |= (1<<LED))
#define LED_HIGH()      (LED_PORT |= (1<<LED))
#define LED_LOW()       (LED_PORT &= ~(1<<LED))
#define LED_TOGGLE()    (LED_PORT ^= (1<<LED))

// For when receiving usb msg on control endpoint 0
#define USB_LED_OFF     0
#define USB_LED_ON      1
#define USB_DATA_OUT    2

// pushbutton init
#define PB_INIT()       PORTB = (1<<PB3)


#endif
