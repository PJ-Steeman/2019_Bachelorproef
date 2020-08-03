/*
 * stuurEnOntvang.c
 *
 * Created: 12/05/2019 1:19:58
 * Author : samde
 */

#define F_CPU 16000000
#define BAUD1 9600
#define BRC1 ((F_CPU/16/BAUD1)-1) // 103 decimaal

#include <avr/io.h>
#include <util/delay.h>
#include <string.h>
#include <avr/interrupt.h>

#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#define RX_BUFFER_SIZE  128  //even getal want kan 2 chars storen?

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
char atCommand_6[] = "AT+CNMI=1,2,0,0,0\r";
char atCommand_7[] = "Alarm is uitgeschakeld, meester\r";

char atRespons_1[] = "OK";
char atRespons_2[] = "OK";
char atRespons_3[] = ">";


void stuurAT(char atCommand[]);
void stuurAlarmSMS(char boodschap[]);
void initSMSModule(void);
void bekijkIngang(void);
int checkOK(char respons[]);
int checkSMSCode(void);
char getChar(void);
char ontvangChar(void);

//NB: de '\0'in de rxBuffer kan een probleem zijn bij de strlen controle want die checkt
// op '\0' dacht ik
int main(void)
{
	UBRR1H = 0;
	UBRR1L = BRC1;

	UCSR1B = (1 << RXCIE1) | (1 << RXEN1)  | (1 << TXEN1);
	UCSR1C = (1 << UCSZ01) | (1 << UCSZ00); 
	DDRB = (1 << PORTB7);

	sei();
	
	sbi(PORTB, PORTB7);
	initSMSModule();
	stuurAlarmSMS(atCommand_4);
	
    while (1)
    {
		_delay_ms(100);
		if (checkSMSCode())
		 {
			cbi(PORTB, PORTB7);
			stuurAlarmSMS(atCommand_7);
			
		 }
    }
}

void stuurAT(char atCommand[])
{
	for ( uint8_t n = 0; n < strlen(atCommand); n++)
	{
		/* Wait for empty transmit buffer */
		while ( !( UCSR1A & (1<<UDRE1)) )
		;
		/* Put data into buffer, sends the data */
		UDR1 = atCommand[n];
		//UDR0 = atCommand[n];
		//_delay_ms(100);
	}
}

char ontvangChar(void) //unsigned char in datasheet
{
	/* Wait for data to be received */
	while ( !(UCSR1A & (1<<RXC1)) )
	;
	/* Get and return received data from buffer */
	return UDR1;
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
int checkSMSCode(void) //deze om de zoveel keer checken in samenprogramma
{
		for (int n = 0; n < strlen(rxBuffer); n++)
		{
			if (rxBuffer[n] == '1')
				if (rxBuffer[n+1] == '2')
					if (rxBuffer[n+2] == '3')
						if (rxBuffer[n+3] == '4')
						{
							memset(rxBuffer, '0', strlen(rxBuffer)); rxWritePos = 0; rxReadPos = 0;
							return 1;
						}
		}
		
		return 0;
}

void stuurAlarmSMS(char boodschap[])
{
	memset(rxBuffer, '0', strlen(rxBuffer)); rxWritePos = 0; rxReadPos = 0;
	stuurAT(atCommand_1); //AT\r
	while(1)
	{
		char c = getChar();
		if (c == 'K')
			break;
	}
	
		/*
		for (int n = 0; n < strlen(rxBuffer); n++)  //dit mag weglaten want OK eindigt op K dus O is ook geweest
		{
			if (rxBuffer[n] == 'O')
			{
				
				if (rxBuffer[n+1] == 'K')
				{
					_delay_ms(5);
					UDR0 = 'K';
				}
				
			}
		}
		*/
		
		memset(rxBuffer, '0', strlen(rxBuffer)); rxWritePos = 0; rxReadPos = 0;
		stuurAT(atCommand_2);  //AT+CMGF=1\r
		while(1)		//enkel hier problemen
		{
			char c = getChar();  // hier wordt +CMGF commando als echo gegeven, dus test op de F
			if (c == 'F')
			{
				break;
			}
						
		}
	
		memset(rxBuffer, '0', strlen(rxBuffer)); rxWritePos = 0; rxReadPos = 0;
		stuurAT(atCommand_3); //AT+CMGS="+324....."\r
		
		while(1)
		{
			char c = getChar();
			if (c == '>')
			break;
		}
		
		memset(rxBuffer, '0', strlen(rxBuffer)); rxWritePos = 0; rxReadPos = 0;
		stuurAT(boodschap);
		stuurAT(atCommand_5);
	
}

/*char codeOntvang(void)
{
	

}
*/

void bekijkIngang(void)
{
	/* Wait for data to be received */
	while ( !(UCSR1A & (1<<RXC1)) )
	;
	/* Get and return received data from buffer */
	rxBuffer[rxWritePos] = UDR1;
	
	
	
	/* Wait for empty transmit buffer */
	while ( !( UCSR1A & (1<<UDRE1)) )
	;
	/* Put data into buffer, sends the data */
	UDR1 = rxBuffer[rxWritePos];
	
	rxWritePos++;
}

void initSMSModule(void)
{
	memset(rxBuffer, '0', strlen(rxBuffer)); rxWritePos = 0; rxReadPos = 0;
	stuurAT(atCommand_1); //AT\r
	while(1)
	{
		char c = getChar();
		if (c == 'K')
			break;
	}
	
		memset(rxBuffer, '0', strlen(rxBuffer)); rxWritePos = 0; rxReadPos = 0;
		stuurAT(atCommand_2);  //AT+CMGF=1\r
		while(1)		//enkel hier problemen
		{
			char c = getChar();
			if (c == 'F')				
				break;
		}
	
		memset(rxBuffer, '0', strlen(rxBuffer)); rxWritePos = 0; rxReadPos = 0;
		stuurAT(atCommand_6); //text ontvang mode zetten
		while(1)		//enkel hier problemen
		{
			char c = getChar();
			if (c == 'I')		//testen op I, want CNMI commando echo
				break;
		}
}



ISR(USART1_RX_vect) // wordt elke keer opgeroepen indien er iets ontvangen wordt in buffer 0
{
	cli();
    rxBuffer[rxWritePos] = UDR1;
    rxWritePos++;

    if(rxWritePos >= RX_BUFFER_SIZE)
    {
        rxWritePos = 0;
    }
	sei();
}


