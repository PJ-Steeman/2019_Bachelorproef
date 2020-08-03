/*
 * AlarmCentrale.c
 */
#define F_CPU 16000000UL

#include <string.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "AlarmCentrale.h"

char IngegevenWachtwoord[4];

int AlarmStatus = 1;		//Of het alarm geactiveerd is of niet (via de code)

int wachtTeller = 0;
int smsTeller = 0;

int ALARM = 0;				//Of het alarm af gaat of niet;
int DAG_NACHT = 0;			//Of het dag of nacht is
int TAMPER = 0;				//Sensor van het tampercontact

int PIR[9] = {0,0,0,0,0,0,0,0,0};

int RELAY_1 = 0;
int RELAY_2 = 0;

int TUIT_1 = 1;				//actief laag
int TUIT_2 = 1;				//actief_laag
int TUIT_3 = 1;				//actief_laag
int TUIT_4 = 1;				//actief_laag

void pinSetup(void)
{
	sei();					//Enable global interupts
	TCCR1A = 0;		
	TCCR1B = 0;		
	
	TCCR1B |= (1 << WGM12);	//Timer in interupt mode
	OCR1A = 62500 - 1;		//Telstand om 1 seconde te krijgen
	TIMSK1 = (1<<OCIE1A);	//Interupt wanneer we aan deze waarde komen
	
	DDRA = 0x00;			//Alle A pinnen als ingangen 
	DDRB = 0B10000000;		//B7 als uitgang
	DDRC = 0B01010101;		//C1,C3,C5,C7 als ingang en C0,C2,C4,C6 als uitgang
	DDRL = 0B00000101;		//Alle L pinnen behalve L0 en L2 als ingangen
	
	PORTA = 0x00;			//Pull-Up weerstanden uitzetten op de A-pinnen
	PORTC = 0B01010101;		//Pull-Up weerstanden uitzetten op de C-pinnen en alle uitgangen op 1 zetten
	PORTL = 0B00000101;			//Pull-Up weerstanden uitzetten op de L-pinnen en alle uitgangen op 1 zetten
	
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

void alarmGaatAf(int stat)
{
	if (stat)
	{
		LED_ON
		tranUitgangen(1);
		relayUitgangen(1);
		//SIRENE_ON
		//SEND_SMS
	} 
	else
	{
		LED_OFF
		tranUitgangen(0);
		relayUitgangen(0);
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
		alarmGaatAf(0);
	}
}

int main(void)
{
	pinSetup();
	
	memset(IngegevenWachtwoord,'\0',strlen(IngegevenWachtwoord));
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
			if(GEBRUIK_ZONES == 1){
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
		
	}
}

//INTERUPT ROUTINE
ISR(TIMER1_COMPA_vect){
	wachtTeller ++;
	LED_TOGGLE;		
}


