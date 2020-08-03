/*
 * stuurEnOntvang.c
 *
 * Created: 12/05/2019 1:19:58
 * Author : samde
 */

#define F_CPU 16000000
#define BAUD 9600
#define BRC ((F_CPU/16/BAUD)-1) // 103 decimaal

#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include <avr/interrupt.h>

#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#define RX_BUFFER_SIZE  128

uint8_t rxReadPos = 0;
uint8_t rxWritePos = 0;
uint8_t i = 0;
char rxBuffer[RX_BUFFER_SIZE];
char respons[20] = {'\0'};

char atCommand_1[] = "AT\r";
char atCommand_2[] = "AT+CMGF=1\r";
char atCommand_3[] = "AT+CMGS=\"+32494069280\"\r";
char atCommand_4[] = "Testertest\r";
char atCommand_5[] = "\x1a\r";

char atRespons_1[] = "OK";
char atRespons_2[] = "OK";
char atRespons_3[] = ">";


void stuurSMS(char atCommand[]);
void stuurAlarm(void);
void bekijkIngang(void);
int checkOK(char respons[]);
char getChar(void);
char ontvangChar(void);


int main(void)
{
	UBRR0H = 0;
	UBRR0L = BRC;

	UCSR0B = (1 << RXCIE0) | (1 << RXEN0)  | (1 << TXEN0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
	DDRB = (1 << PORTB7);

	sei();

	stuurAlarm();
    while (1)
    {
    }
}

void stuurSMS(char atCommand[])
{
	for ( uint8_t n = 0; n < strlen(atCommand); n++)
	{
		/* Wait for empty transmit buffer */
		while ( !( UCSR0A & (1<<UDRE0)) )
		;
		/* Put data into buffer, sends the data */
		UDR0 = atCommand[n];
		//UDR0 = atCommand[n];
		//_delay_ms(100);
	}
}

char ontvangChar(void) //unsigned char in datasheet
{
	/* Wait for data to be received */
	while ( !(UCSR0A & (1<<RXC0)) )
	;
	/* Get and return received data from buffer */
	return UDR0;
}


char getChar(void)
{
	char ret = '\0';

	if(rxReadPos != rxWritePos)
	{
		ret = rxBuffer[rxReadPos];

		rxReadPos++;

		if(rxReadPos >= RX_BUFFER_SIZE)
		{
			rxReadPos = 0;
		}
	}

	return ret;
}

void stuurAlarm(void)
{
	stuurSMS(atCommand_1); //constant checken
	while(1)
	{
		char c = getChar();
		if (c == 'K')
			UDR0 = 'K';
	}
	
		UDR0 = 'O';
		for (int n = 0; n < strlen(rxBuffer); n++)
		{
			if (rxBuffer[i] == 'O')
			{
				
				if (rxBuffer[i+1] == 'K')
			}
		}
		stuurSMS(atCommand_2);
	if (atRespons_2[0] == respons[2] && atRespons_2[1] == respons[3]) // 3 en 4 ipv 2 en 3 vanwege \r
	{
		stuurSMS(atCommand_3);
		
	
	}
	if (atRespons_3[0] == respons[4])
	{
		stuurSMS(atCommand_4);
		stuurSMS(atCommand_5);
	}
}

/*char codeOntvang(void)
{
	

}
*/

void bekijkIngang(void)
{
	/* Wait for data to be received */
	while ( !(UCSR0A & (1<<RXC0)) )
	;
	/* Get and return received data from buffer */
	rxBuffer[rxWritePos] = UDR0;
	
	
	
	/* Wait for empty transmit buffer */
	while ( !( UCSR0A & (1<<UDRE0)) )
	;
	/* Put data into buffer, sends the data */
	UDR0 = rxBuffer[rxWritePos];
	
	rxWritePos++;
}

int checkOK(char respons[])
{
	for(int k = 0; k < strlen(respons); k++)
	{
		if (respons[k] == 'O')
		{
			if (respons[k+1] == 'K')
			{
				return 1;
			}
		}
	}
	
	return 0;
}



ISR(USART0_RX_vect) // wordt elke keer opgeroepen indien er iets ontvangen wordt
{
	cli();
    rxBuffer[rxWritePos] = UDR0;
    rxWritePos++;

    if(rxWritePos >= RX_BUFFER_SIZE)
    {
        rxWritePos = 0;
    }
	sei();
}


