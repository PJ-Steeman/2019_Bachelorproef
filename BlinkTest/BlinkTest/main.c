#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

int main(void)
{
    DDRB |= 0B10000000; 
    while (1) 
    {
		PORTB |= 0B10000000;
		_delay_ms(250);
		PORTB &= 0B01111111;
		_delay_ms(250);
    }
}

