/*
 * NANOresponder.c
 *
 * Created: 18/04/2019 16:41:35
 * Author : Maarten Colignon
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>


#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))

#define F_CPU	16000000
#define BAUD	57600
#define BRC		((F_CPU/16/BAUD) - 1)
 
#define AANTAL_ZONES	8

#define READ_BUFFER_SIZE	16

#define BERICHT_LENGTE 14
char bericht[BERICHT_LENGTE] = "ontvang";
char ontvangenBericht[BERICHT_LENGTE];
char rxBuffer[READ_BUFFER_SIZE];
int rxReadPos = 0;
int rxWritePos = 0;

int CODE = 0;
char code[] = {1,2,3,4};

int zoneStatus[AANTAL_ZONES] = {0,0,0,0,0,0,0,0};
char masterStatus[2];
char zoneChar[2];
int zoneGetal;
int berichtOnbekend = 0;
int ALARM = 0;

char icode[4];
int codeteller;

int iterato = 0;

#define TX_BUFFER_SIZE	16

#include <util/delay.h>
#include <string.h>

char serialBuffer[TX_BUFFER_SIZE];
uint8_t serialReadPos = 0;
uint8_t serialWritePos = 0;

char antwoord[BERICHT_LENGTE];

void startVerzenden();
void send(char c[]);
void clear_buffer();
char getChar(void);
//char peekChar(void);
//void appendSerial(char c);
//void serialWrite(char c[]);
//void decodeerBericht(void);
//void decodeerZones(int decWaarde);
//void zoneTest(void);
char *maakBericht(char zin[]);
char leesIn ();

int main(void)
{
	UBRR0H = (BRC >> 8);
	UBRR0L = BRC;
	
	UCSR0B = (1 << RXEN0) | (1 << RXCIE0) | (1 << TXEN0) | (1 << TXCIE0);
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
	
	DDRB = 0B00000100;
	DDRB = (1 << PORTB0);
	
	
	// DDRD |= 0x02; // PD2:Dir
	// PORTD &= ~0x02;
	
	char c = 'F';
	char vorig = 'F';
	
	sei();
	clear_buffer();
	while(1)
	{
		c = leesIn(); // hier lees ik mijn character in van de keypad
		if (c != vorig){
			if (c == 'B')
			{
				//state = zetAanZone();
			}
			else if (c == 'A')
			{
				//state = zetAan();
			}
			else if (c == 'F')
			{
			}
			else if ((c == 'C')| (c == 'D') | (c == '*')| (c == '#'))
			{
				//zetAf();
				break;
			}
			else
			{
				//hier komen we als er een getal ingelezen wordt
				icode[codeteller] = c;
				codeteller++;
			}
			vorig = c;
			if (codeteller == 4)
			{
				codeteller = 0;
				CODE = 1;
			}
		}
		
		char in = getChar();
		
		if(in == '\n')
		{
			strcpy(ontvangenBericht, rxBuffer);
			//send(ontvangenBericht);
			//UDR0 = ontvangenBericht[0];
			//send(rxBuffer);
			
			for ( int i = 0; i<READ_BUFFER_SIZE; i++)
			{
				if ( rxBuffer[i] == '$' )
				{
					if ( rxBuffer[i+1] == 'N' )
					{
						masterStatus[0] = ontvangenBericht[i+2];
						masterStatus[1] = ontvangenBericht[i+3];
						//send(masterStatus);
						zoneChar[0] = ontvangenBericht[i+4];
						zoneChar[1] = ontvangenBericht[i+5];
						//send(zoneChar);
						
						/*
						zoneGetal =  atoi(zoneChar);
						decodeerZones(zoneGetal);
						*/
						
						//if (strncmp("AA", masterStatus, strlen(masterStatus)))
						if ( rxBuffer[i+2] == 'A' && rxBuffer[i+3] == 'A')
						{
							ALARM = 1;
							//UDR0 = 'A';
						}
						//else if (strncmp("US", masterStatus, 2))
						else if ( rxBuffer[i+2] == 'U' && rxBuffer[i+3] == 'S')
						{
							ALARM = 0;
							//UDR0 = 'B';
						}
						else
						{
							// Bericht is onbekend
							break;
						}
						
						startVerzenden();
						
					}
					else
					{
						break;
					}
				}		
			}
			clear_buffer();
		}
	}
}

void startVerzenden()
{
	maakBericht(antwoord);
	PORTB |= 0B00000100;
	//UDR0 = 'T';
	cli();
	for (int t=0; t<strlen(antwoord); t++)
	{
		UDR0 = antwoord[t];
		_delay_ms(5);
	}
	//send("ant\r");
	//send(antwoord);
	PORTB &= 0B11111011;
	sei();
}

void send(char c[])
{
	cli();
	for(uint8_t i = 0; i < strlen(c); i++)
	{
		UDR0 = c[i];
		_delay_ms(5);
	}
	/*
	if(UCSR0A & (1 << UDRE0))
	{
		UDR0 = 0;
	}*/
	sei();
}

void clear_buffer()
{
	memset(rxBuffer, '\0', sizeof(rxBuffer));
	rxReadPos = 0;
	rxWritePos = 0;
	memset(antwoord, '\0', sizeof(antwoord));
}

/*
char peekChar(void)
{
	char ret = '\0';
	
	if(rxReadPos != rxWritePos)
	{
		ret = rxBuffer[rxReadPos];
	}
	
	return ret;
}
*/
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

/*
void appendSerial(char c)
{
	serialBuffer[serialWritePos] = c;
	serialWritePos++;
	
	if(serialWritePos >= TX_BUFFER_SIZE)
	{
		serialWritePos = 0;
	}
}

void serialWrite(char c[])
{
	for(uint8_t i = 0; i < strlen(c); i++)
	{
		appendSerial(c[i]);
	}
	
	if(UCSR0A & (1 << UDRE0))
	{
		UDR0 = 0;
	}
}


void decodeerBericht(void)
{

}


void decodeerZones(int decWaarde)
{
	for(int i=0; i<AANTAL_ZONES; i++)
	{
		if (decWaarde%2 == 1)
			zoneStatus[i] = 1;
		else
			zoneStatus[i] = 0;
		decWaarde/=2;
	}
}

void zoneTest(void)
{
	for(int i=0; i<AANTAL_ZONES; i++)
	{
		UDR0 = zoneStatus[i];
	}
}
*/
char *maakBericht(char zin[])
{	
	zin[0] = '$'; //Startkarakter
	zin[1] = 'M'; //Master adres
	
	if (CODE == 1)
	{
		zin[2] = 'C'; //CD : Er is een code beschikbaar
		zin[3] = 'D';
		
		zin[4] = code[0]; //Code characters
		zin[5] = code[1];
		zin[6] = code[2];
		zin[7] = code[3];
	}
	else
	{
		zin[2] = 'O'; //US : Update Status  (poll naar de status van de slave)
		zin[3] = 'K';
		
		zin[4] = '0'; //Code characters
		zin[5] = '0';
		zin[6] = '0';
		zin[7] = '0';
	}
	
	zin[8] = '#'; //Eindkarakter
	zin[9] = '\n';
	
	return zin;
}

char leesIn (){
	
	// D9 hoog zetten
	PORTB = 0B00000010;
	_delay_ms(5);
	// toestand van D2 lezen
	if(PIND & 0B00000100){
		PORTB = 0B00000000;
		return '*';
	}
	// toestand van D3 lezen
	else if(PIND & 0B00001000){
		PORTB = 0B00000000;
		return '7';
	}
	// toestand van D4 lezen
	else if(PIND & 0B00010000){
		PORTB = 0B00000000;
		return '4';
	}
	// toestand van D5 lezen
	else if(PIND & 0B00100000){
		PORTB = 0B00000000;
		return '1';
	}
	
	// D8 hoog zetten en D9 laag
	PORTB = 0B00000001;
	_delay_ms(5);
	// toestand van D2 lezen
	if(PIND & 0B00000100){
		PORTB = 0B00000000;
		//moet 0 zijn, maar voor de test is het 20
		return '0';
	}
	// toestand van D3 lezen
	else if(PIND & 0B00001000){
		PORTB = 0B00000000;
		return '8';
	}
	// toestand van D4 lezen
	else if(PIND & 0B00010000){
		PORTB = 0B00000000;
		return '5';
	}
	// toestand van D5 lezen
	else if(PIND & 0B00100000){
		PORTB = 0B00000000;
		return '2';
	}
	
	// D8 laag zetten, en D7 hoog zetten
	PORTB = 0B00000000;
	PORTD = 0B10000000;
	_delay_ms(5);
	// toestand van D2 lezen
	if(PIND & 0B00000100){
		PORTD = 0B00000000;
		return '#';
	}
	// toestand van D3 lezen
	else if(PIND & 0B00001000){
		PORTD = 0B00000000;
		return '9';
	}
	// toestand van D4 lezen
	else if(PIND & 0B00010000){
		PORTD = 0B00000000;
		return '6';
	}
	// toestand van D5 lezen
	else if(PIND & 0B00100000){
		PORTD = 0B00000000;
		return '3';
	}
	
	// D7 laag zetten en D6 hoog zetten
	PORTD = 0B01000000;
	_delay_ms(5);
	// toestand van D2 lezen
	if(PIND & 0B00000100){
		PORTD = 0B00000000;
		return 'D';
	}
	// toestand van D3 lezen
	else if(PIND & 0B00001000){
		PORTD = 0B00000000;
		return 'C';
	}
	// toestand van D4 lezen
	else if(PIND & 0B00010000){
		PORTD = 0B00000000;
		return 'B';
	}
	// toestand van D5 lezen
	else if(PIND & 0B00100000){
		PORTD = 0B00000000;
		return 'A';
	}
	
	// D6 terug laag zetten
	PORTD = 0B00000000;
	return 'F';
}

	

ISR(USART_RX_vect)
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

