/*
 * File: timer1.c
 * Project: StickPass
 * Author: Alexandru Jora (alexandru@jora.ca)
 * Creation Date: 2016-02-22
 * License: GNU GPL v3 (see LICENSE)
 *
 */

#include <avr/interrupt.h>
#include "timer1.h"

volatile unsigned char counter100ms = 0;

// interrupt routine for timer1 every 100ms
ISR(TIM1_OVF_vect) {
    TCNT1 = 55;

    // increment 100ms counter
    counter100ms++;
    // reset counter after 2s
    if(counter100ms == 20)
        counter100ms = 0;
}

/*
 * Timer1 initialization
 * Prescaler = 8192
 * Overflow 10x per second (100ms)
 *
 */
void timer1_Init(void) {
    // timer1 prescaler select (datasheet p.89)
    TCCR1 |=  (1<<CS13)|(1<<CS12)|(1<<CS11);
    TCCR1 &= ~(1<<CS10);

    // timer1 interrupt enable (datasheet p.81)
    TIMSK |= (1<<TOIE1);

    // reset initial values
    counter100ms = 0;
    TCNT1 = 0;

}
