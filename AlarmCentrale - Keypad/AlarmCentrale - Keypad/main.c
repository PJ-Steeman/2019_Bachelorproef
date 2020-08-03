/*
 * NANOresponder.c
 *
 * Created: 18/04/2019 16:41:35
 * Author : Maarten Colignon
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>

#define F_CPU	16000000
#define BAUD	57600
#define BRC		((F_CPU/16/BAUD) - 1)
 
#define AANTAL_ZONES	8

#define READ_BUFFER_SIZE	16

#define BERICHT_LENGTE 14
char rxBuffer[READ_BUFFER_SIZE];
int rxReadPos = 0;
int rxWritePos = 0;

int CODE = 0;
char code[] = {1,2,3,4};

#define TX_BUFFER_SIZE	16

#include <util/delay.h>
#include <string.h>

char serialBuffer[TX_BUFFER_SIZE];
uint8_t serialReadPos = 0;
uint8_t serialWritePos = 0;

char antwoord[] = "halloow";

void startVerzenden();
void clear_buffer();
char getChar(void);

int main(void)
{
	UBRR0H = (BRC >> 8);
	UBRR0L = BRC;
	
	UCSR0B = (1 << RXEN0) | (1 << RXCIE0) | (1 << TXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
	
	DDRB = 0B10000000;

	
	sei();
	clear_buffer();
	while(1)
	{
		char c = getChar();
		
		if(c == '\n')
		{
			UDR0 = 'W';
			startVerzenden();
			clear_buffer();
		}
	}
}

void startVerzenden()
{
	cli();
	for (int t=0; t<strlen(antwoord); t++)
	{
		UDR0 = antwoord[t];
		_delay_ms(5);
	}
	sei();
}


void clear_buffer()
{
	memset(rxBuffer, '\0', sizeof(rxBuffer));
	rxReadPos = 0;
	rxWritePos = 0;
	memset(antwoord, '\0', sizeof(antwoord));
}

char getChar(void)
{
	char ret = '\0';
	
	if(rxReadPos != rxWritePos)
	{
		ret = rxBuffer[rxReadPos];
		
		rxReadPos++;
		
		if(rxReadPos >= READ_BUFFER_SIZE)
		{
			rxReadPos = 0;
		}
	}
	return ret;
}

ISR(USART0_RX_vect)
{
	rxBuffer[rxWritePos] = UDR0;
	//UDR0 = 'E';
	//UDR0 = rxBuffer[rxWritePos];
	rxWritePos++;
	
	if(rxWritePos >= READ_BUFFER_SIZE)
	{
		rxWritePos = 0;
	}
}

