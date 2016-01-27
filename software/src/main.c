#include <avr/io.h>
#include <util/delay.h>

int main() {
        DDRB |= 1; // LED on PB0
        while(1) {
                PORTB |= 1; // Turn LED on
                _delay_ms(500);
                PORTB &= ~1; // Turn LED off
                _delay_ms(500);
        }

        return 1;
}
