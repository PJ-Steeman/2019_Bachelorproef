/*
 * AlarmCentrale.c
 */
#define F_CPU	16000000UL
#define BAUD	57600
#define BRC		((F_CPU/16/BAUD) - 1)
#define BAUD1	9600
#define BRC1	((F_CPU/16/BAUD1)-1) // 103 decimaal

#include <string.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "AlarmCentrale.h"

#define RX_BUFFER_SIZE  128

uint8_t rxReadPos = 0;
uint8_t rxWritePos = 0;
uint8_t i = 0;
char rx1Buffer[RX_BUFFER_SIZE];
char respons[20] = {'\0'};

char atCommand_1[] = "AT\r";
char atCommand_2[] = "AT+CMGF=1\r";
char atCommand_3[] = "AT+CMGS=\"+32494069280\"\r";
char boodschap1[] = "Alarm gaat af\r";
char atCommand_5[] = "\x1a\r";
char atCommand_6[] = "AT+CNMI=1,2,0,0,0\r";
char boodschap2[] = "Alarm is uitgeschakeld, meester\r";

char atRespons_1[] = "OK";
char atRespons_2[] = "OK";
char atRespons_3[] = ">";

char IngegevenWachtwoord[4];

int AlarmStatus = 1;		//Of het alarm geactiveerd is of niet (via de code)
int smsUitFlag = 0;

int wachtTeller = 0;
int smsTeller = 0;

long iteratieTeller = 0;

int ALARM = 0;				//Of het alarm af gaat of niet;
int DAG_NACHT = 0;			//Of het dag of nacht is
int TAMPER = 0;				//Sensor van het tampercontact

#define AANTAL_SENSORS 9
int PIR[AANTAL_SENSORS] = {0,0,0,0,0,0,0,0,0};

int RELAY_1 = 0;
int RELAY_2 = 0;

int TUIT_1 = 1;				//actief laag
int TUIT_2 = 1;				//actief_laag
int TUIT_3 = 1;				//actief_laag
int TUIT_4 = 1;				//actief_laag

int gebruikZones = 0;
int smsFlag = 0;

char txBuffer[TX_BUFFER_SIZE];
int txLeesPos = 0;
int txSchrijfPos = 0;
char bericht[TX_BUFFER_SIZE-1];

char rxBuffer[LEES_BUFFER_SIZE];
int rxLeesPos = 0;
int rxSchrijfPos = 0;

int zoneWaarde;
char zoneChars[6];

char ontvangenBericht[BERICHT_LENGTE];


void pinSetup(void)
{
	sei();					//Enable global interupts
	TCCR1A = 0;		
	TCCR1B = 0;		
	
	TCCR1B |= (1 << WGM12);	//Timer in interupt mode
	OCR1A = 62500 - 1;		//Telstand om 1 seconde te krijgen
	TIMSK1 = (1<<OCIE1A);	//Interupt wanneer we aan deze waarde komen
	
	DDRA = 0x00;			//Alle A pinnen als ingangen 
	DDRB = 0B11000000;		//B7 als uitgang
	DDRC = 0B01010101;		//C1,C3,C5,C7 als ingang en C0,C2,C4,C6 als uitgang
	DDRL = 0B00000101;		//Alle L pinnen behalve L0 en L2 als ingangen
	
	PORTA = 0x00;			//Pull-Up weerstanden uitzetten op de A-pinnen
	PORTC = 0B01010101;		//Pull-Up weerstanden uitzetten op de C-pinnen en alle uitgangen op 1 zetten
	PORTL = 0B00000101;			//Pull-Up weerstanden uitzetten op de L-pinnen en alle uitgangen op 1 zetten
	
	UBRR0H = (BRC >> 8);
	UBRR0L =  BRC;
	
	UCSR0B = (1 << RXEN0) | (1 << RXCIE0) | (1 << TXEN0) /* | (1 << TXCIE0)*/;
	
	UBRR1H = 0;
	UBRR1L = BRC1;

	UCSR1B = (1 << RXCIE1) | (1 << RXEN1)  | (1 << TXEN1);
	UCSR1C = (1 << UCSZ01) | (1 << UCSZ00);
	
	
	memset(&txBuffer[0], '\0', sizeof(txBuffer));
	
}

void tranUitgangen(int stat)
{
	if(stat)
	{
		PORTC &= 0B10101010;	//alle low-voltage transistor uitgangen op 0(ON)
	}
	else
	{
		PORTC |= 0B01010101;	//alle low-voltage transistor uitgangen op 1(OFF)
	}
}

void relayUitgangen(int stat)
{
	if(stat)
	{
		PORTL &= 0B11111010;
	}
	else
	{
		PORTL |= 0B00000101;
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
		ret = rx1Buffer[rxReadPos];

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
		for (int n = 0; n < strlen(rx1Buffer); n++)
		{
			if (rx1Buffer[n] == WACHTWOORD[0])
				if (rx1Buffer[n+1] == WACHTWOORD[1])
					if (rx1Buffer[n+2] == WACHTWOORD[2])
						if (rx1Buffer[n+3] == WACHTWOORD[3])
						{
							memset(rx1Buffer, '0', strlen(rx1Buffer)); rxWritePos = 0; rxReadPos = 0;
							return 1;
						}
		}
		
		return 0;
}

void stuurAlarmSMS(char boodschap[])
{
	memset(rx1Buffer, '0', strlen(rx1Buffer)); rxWritePos = 0; rxReadPos = 0;
	stuurAT(atCommand_1); //AT\r
	int n = 0;
	while(1)
	{
		char c = getChar();
		if (c == 'K')
			break;
		_delay_ms(10);
		if (n >= 100)
			break;
		n++;
	}
	
		/*
		for (int n = 0; n < strlen(rx1Buffer); n++)  //dit mag weglaten want OK eindigt op K dus O is ook geweest
		{
			if (rx1Buffer[n] == 'O')
			{
				
				if (rx1Buffer[n+1] == 'K')
				{
					_delay_ms(5);
					UDR0 = 'K';
				}
				
			}
		}
		*/
		
		memset(rx1Buffer, '0', strlen(rx1Buffer)); rxWritePos = 0; rxReadPos = 0;
		stuurAT(atCommand_2);  //AT+CMGF=1\r
		n = 0;
		while(1)		//enkel hier problemen
		{
			char c = getChar();  // hier wordt +CMGF commando als echo gegeven, dus test op de F
			if (c == 'F')
				break;
			_delay_ms(10);
			if (n >= 100)
				break;
			n++;
						
		}
	
		memset(rx1Buffer, '0', strlen(rx1Buffer)); rxWritePos = 0; rxReadPos = 0;
		stuurAT(atCommand_3); //AT+CMGS="+324....."\r
		n = 0;
		while(1)
		{
			char c = getChar();
			if (c == '>')
				break;
			if (n >= 100)
				break;
			n++;
		}
		
		memset(rx1Buffer, '0', strlen(rx1Buffer)); rxWritePos = 0; rxReadPos = 0;
		stuurAT(boodschap);
		stuurAT(atCommand_5);
	
}

void initSMSModule(void)
{
	memset(rx1Buffer, '0', strlen(rx1Buffer)); rxWritePos = 0; rxReadPos = 0;
	stuurAT(atCommand_1); //AT\r
	int n=0;
	while(1)
	{
		char c = getChar();
		if (c == 'K')
			break;
		_delay_ms(10);  //dit is failsafe voor als de gsm module niet werkt, maar zo kan alarm nog wel werken
		if (n>=100)
			break;
		n++;
	}
	
	memset(rx1Buffer, '0', strlen(rx1Buffer)); rxWritePos = 0; rxReadPos = 0;
	stuurAT(atCommand_2);  //AT+CMGF=1\r
	n=0;
	while(1)		//enkel hier problemen
	{
		char c = getChar();
		if (c == 'F')
			break;
		_delay_ms(10);
		if (n>=100)
			break;
		n++;
	}
	
	memset(rx1Buffer, '0', strlen(rx1Buffer)); rxWritePos = 0; rxReadPos = 0;
	stuurAT(atCommand_6); //text ontvang mode zetten
	while(1)		//enkel hier problemen
	{
		char c = getChar();
		if (c == 'I')		//testen op I, want CNMI commando echo
			break;
		_delay_ms(10);
		if (n>=100)
			break;
		n++;
	}
}


void alarmGaatAf(int stat)
{
	if (stat)
	{
		LED_ON
		tranUitgangen(1);
		relayUitgangen(1);
		ALARM = 1;
		smsUitFlag = 0;
		if(!smsFlag)
		{
			stuurAlarmSMS(boodschap1);
			smsFlag = 1;
		}
		
		//SIRENE_ON
		//SEND_SMS
	} 
	else
	{
		LED_OFF
		tranUitgangen(0);
		relayUitgangen(0);
		ALARM = 0;
		smsFlag = 0;
		if ( !smsUitFlag)
		{
			stuurAlarmSMS(boodschap2);
			smsUitFlag = 1;
		}
	}
}

void disable_enableAlarm(int stat)
{
	if (stat)
	{
		AlarmStatus = 1;
	}
	else
	{
		AlarmStatus = 0;
		ALARM = 0;
		alarmGaatAf(0);
	}
}

void leeg_buffer()
{
	memset(txBuffer, '\0', strlen(txBuffer));
	txSchrijfPos = 0;
	txLeesPos = 0;
}

int berekenZoneWaarde()
{
	int uitkomst = 0;
	int vermenigvuldiger = 1;
	int i;
	for (i=AANTAL_SENSORS-1; i>=1; i--)
	{
		uitkomst += PIR[i] * vermenigvuldiger;
		vermenigvuldiger *= 2;
	}
	return uitkomst;
}

char *maakBericht(char zin[])
{
	zoneWaarde = berekenZoneWaarde(); //Haalt huidige zoneStatus op in integervorm
	//if
	snprintf( zoneChars, 6, "%d", zoneWaarde );	//Conversie naar char-array
	//send(zoneChars);
	
	zin[0] = '$'; //Startkarakter
	zin[1] = 'N'; //Slave adres
	
	if ( ALARM == 1 )
	{
		zin[2] = 'A'; //US : Update Status  (poll naar de status van de slave)
		zin[3] = 'A';
		
		if ( zoneWaarde < 10 )
		{
			zin[4] = '0';
			zin[5] = zoneChars[0];
		}
		else
		{
			zin[4] = zoneChars[0]; //Status in char vorm in het update-bericht
			zin[5] = zoneChars[1];
		}
		
		//zin[6] = zoneChars[2];
	}
	else
	{
		zin[2] = 'U'; //US : Update Status  (poll naar de status van de slave)
		zin[3] = 'S';
		
		if ( zoneWaarde == 0 )
		{
			zin[4] = '0';
			zin[5] = '0';
		}
		else if ( zoneWaarde < 10 )
		{
			zin[4] = '0';
			zin[5] = zoneChars[0];
		}
		else
		{
			zin[4] = zoneChars[0]; //Status in char vorm in het update-bericht
			zin[5] = zoneChars[1];
		}
		//zin[6] = zoneChars[2];
	}
	
	zin[6] = '#'; //Eindkarakter
	zin[7] = '\n';
	
	return zin;
}

void verstuur(char c[])
{
	for(uint8_t i = 0; i < strlen(c); i++)
	{
		UDR0 = c[i];
		_delay_ms(5);
	}
	
	if(UCSR0A & (1 << UDRE0))
	{
		UDR0 = 0;
	}
}

void start_communicatie()
{
	leeg_buffer();
	maakBericht(bericht);
	PORTB |= 0B01000000;
	cli();
	verstuur(bericht);
	PORTB &= 0B10111111;
	sei();
}

void controleerAntwoord()
{
	strcpy(ontvangenBericht, rxBuffer);
	for ( int i = 0; i<LEES_BUFFER_SIZE; i++)
	{
		if ( rxBuffer[i] == '$' )
		{
			if ( rxBuffer[i+1] == 'M' )
			{
				//if (strncmp("AA", masterStatus, strlen(masterStatus)))
				if ( rxBuffer[i+2] == 'C' && rxBuffer[i+3] == 'D')
				{
					IngegevenWachtwoord[0] = ontvangenBericht[i+2];
					IngegevenWachtwoord[1] = ontvangenBericht[i+3];
					IngegevenWachtwoord[2] = ontvangenBericht[i+4];
					IngegevenWachtwoord[3] = ontvangenBericht[i+5];
					//UDR0 = 'A';
				}
				//else if (strncmp("US", masterStatus, 2))
				else if ( rxBuffer[i+2] == 'O' && rxBuffer[i+3] == 'K')
				{
					break;
					//UDR0 = 'B';
				}
				else
				{
					// Bericht is onbekend
					break;
				}
			}
			else
			{
				break;
			}
		}
	}
}
		


int main(void)
{
	pinSetup();
	initSMSModule();
	
	
	memset(IngegevenWachtwoord,'\0',strlen(IngegevenWachtwoord));
	memset(&txBuffer[0], '\0', sizeof(txBuffer));
	int kansen = 3;

	struct zone Zones[NR_ZONES];
	for(int t = 0; t < NR_ZONES; t++){
		for(int y = 0; y < NR_SENSOREN_PER_ZONE; y++){
			Zones[t].sensor[y] = LEEG;
		}
	}
	
	//keuken
	Zones[0].id = 0;
	Zones[0].sensor[0] = 1;
	Zones[0].sensor[1] = 2;
	Zones[0].sensor[2] = 3;
	//living
	Zones[1].id = 1;
	Zones[1].sensor[0] = 4;
	Zones[1].sensor[1] = 5;
	Zones[1].sensor[2] = 6;
	//gang
	Zones[2].id = 2;
	Zones[2].sensor[0] = 7;
	Zones[2].sensor[1] = 8;

	struct links Verbindingen[NR_AANLIGGENDE_ZONES];
	//keuken <-> living
	Verbindingen[0].aanliggend[0] = Zones[0];
	Verbindingen[0].aanliggend[1] = Zones[1];
	//living <-> gang
	Verbindingen[1].aanliggend[0] = Zones[1];
	Verbindingen[1].aanliggend[1] = Zones[2];

	//controle paneel staat in de keuken
	struct zone VorigeBeweging = Zones[0];
	struct zone HuidigeBeweging = Zones[0];

	int gelinked;
			
    while (1)	//oneindige lus
    {
		PIR[1]		= !(PINA & (1<<PINA0));		//PIN 22
		PIR[2]		= !(PINA & (1<<PINA2));		//PIN 24
		PIR[3]		= !(PINA & (1<<PINA4));		//PIN 26
		PIR[4]		= !(PINA & (1<<PINA6));		//PIN 28
		PIR[5]		= !(PINC & (1<<PINC7));		//PIN 30
		PIR[6]		= !(PINC & (1<<PINC5));		//PIN 32
		PIR[7]		= !(PINC & (1<<PINC3));		//PIN 34
		PIR[8]		= !(PINC & (1<<PINC1));		//PIN 36
		
		TAMPER		= !(PINL & (1<<PINL3));		//PIN 46
		DAG_NACHT	= PINL & (1<<PINL1);		//PIN 48

		//Als het alarm actief is EN 1 van de sensoren detecteerd beweging
		if ((PIR[1] || PIR[2] || PIR[3] || PIR[4] || PIR[5] || PIR[6] || PIR[7] || PIR[8]) && AlarmStatus)
		{
			//bepaal de zone van de beweging
			if(gebruikZones == 1){
				for(int i = 0; i < NR_ZONES; i++)
				{
					for(int j = 0; Zones[i].sensor[j] != LEEG; j++)
					{
						if(PIR[Zones[i].sensor[j]])
						{
							VorigeBeweging = HuidigeBeweging;
							HuidigeBeweging = Zones[i];
						}
					}
				}

				gelinked = 0;
				//bepaal of de huidige zone en de vorige verbonden zijn
				for(int i = 0; i < NR_AANLIGGENDE_ZONES; i++)
				{
					int teller = 0;
					for(int j = 0; j < NR_ZONES_IN_LINK; j++)
					{
						if(Verbindingen[i].aanliggend[j].id == HuidigeBeweging.id || Verbindingen[i].aanliggend[j].id == VorigeBeweging.id)
						{
							teller++;
						}
						if(teller == 2)
						{
							gelinked = 1;
						}
					}
				}

				if(!(VorigeBeweging.id == HuidigeBeweging.id || gelinked == 1)){
					START_TIMER;
				}
			}
			else
			{
				START_TIMER;
			}
		}

		if (IngegevenWachtwoord[3] != '\0' && AlarmStatus)
		{
			//als het ingegeven wachtwoord overeenkomt met het juiste wachtwoord
			if(strcmp(WACHTWOORD, IngegevenWachtwoord) == 0){
				{
					disable_enableAlarm(0);
					memset(IngegevenWachtwoord,'\0',strlen(IngegevenWachtwoord));
				}
			}
			else
			{
				memset(IngegevenWachtwoord,'\0',strlen(IngegevenWachtwoord));
				kansen --;
			}

			if(kansen == 0)
			{
				alarmGaatAf(1);
				kansen = 3;
			}
		}
		
		if(TAMPER && AlarmStatus){
			alarmGaatAf(1);
		}
		
		if (wachtTeller == WACHT_PERIODE){
			STOP_TIMER;
			CLEAR_TIMER;
			alarmGaatAf(1);
			wachtTeller = 0;
		}
		
		iteratieTeller++;
		if ( iteratieTeller == 30000 )
		{
			iteratieTeller = 0;
			start_communicatie();
			if (checkSMSCode())
			{
				disable_enableAlarm(0);
			}
		}
	}
}

//INTERUPT ROUTINE
ISR(TIMER1_COMPA_vect){
	wachtTeller ++;
	LED_TOGGLE;		
}

ISR(USART0_RX_vect)
{
	rxBuffer[rxSchrijfPos] = UDR0;
	//UDR0 = rxBuffer[rxSchrijfPos];
	if ( rxBuffer[rxSchrijfPos] == '\n' )
	{
		controleerAntwoord();
	}
	rxSchrijfPos++;
	
	if(rxSchrijfPos >= LEES_BUFFER_SIZE)
	{
		rxSchrijfPos = 0;
	}
}

ISR(USART1_RX_vect) // wordt elke keer opgeroepen indien er iets ontvangen wordt in buffer 0
{
	cli();
	rx1Buffer[rxWritePos] = UDR1;
	rxWritePos++;

	if(rxWritePos >= RX_BUFFER_SIZE)
	{
		rxWritePos = 0;
	}
	sei();
}


