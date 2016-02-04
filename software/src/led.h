#ifndef LED_H
#define LED_H

/*
 * File: led.h
 * Some general purpose macros to help using LED
 *
 */

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

#endif
