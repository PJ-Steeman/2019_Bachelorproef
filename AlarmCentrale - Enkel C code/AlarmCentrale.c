/*
 * AlarmCentrale.c
 * Author : Pieter-Jan Steeman
 */
#include <string.h>
#include <stdio.h>
#include "AlarmCentrale.h"
#define LEEG '\0'

char IngegevenWachtwoord[4];

int AlarmStatus = 1;		//Of het alarm geactiveerd is of niet (via de code)

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

void tranUitgangen(int stat)
{
	if(stat)
	{
		printf("transistor ON\n");
	}
	else
	{
    printf("transistor OFF\n");
	}
}

void relayUitgangen(int stat)
{
	if(stat)
	{
    printf("relay ON\n");
	}
	else
	{
    printf("relay OFF\n");
	}
}

void alarmGaatAf(int stat)
{
	if (stat)
	{
		tranUitgangen(1);
		relayUitgangen(1);
		printf("alarm gaat af\n");
	}
	else
	{
		tranUitgangen(0);
		relayUitgangen(0);
		printf("alarm is stil\n");
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
	memset(IngegevenWachtwoord,'\0',strlen(IngegevenWachtwoord));

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
	Zones[1].sensor[3] = 7;
	//gang
	Zones[2].id = 2;
	Zones[2].sensor[0] = 8;

	struct links Verbindingen[NR_AANLIGGENDE_ZONES];
	//keuken <-> living
	Verbindingen[0].aanliggend[0] = Zones[0];
	Verbindingen[0].aanliggend[1] = Zones[1];
	//living <-> gang
	Verbindingen[1].aanliggend[0] = Zones[1];
	Verbindingen[1].aanliggend[1] = Zones[2];

	//controle paneel staat in de kueken
	struct zone VorigeBeweging = Zones[0];
	struct zone HuidigeBeweging = Zones[0];

	char pas;
	int pasPlaats = 0;
	int kansen = 3;
	int gelinked;

	while(1)
	{
		//Als het alarm actief is EN 1 van de sensoren detecteerd beweging
		if ((PIR[1] || PIR[2] || PIR[3] || PIR[4] || PIR[5] || PIR[6] || PIR[7] || PIR[8] ) && AlarmStatus)
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

				if(VorigeBeweging.id == HuidigeBeweging.id || gelinked == 1){
					alarmGaatAf(0);
				}
				else{
					alarmGaatAf(1);
				}
			}
			else
			{
				alarmGaatAf(1);
			}
		}

		scanf("%c%*c", &pas);
		IngegevenWachtwoord[pasPlaats] = pas;
		pasPlaats++;
		printf("CODE : %s - PLAATS : %d\n",IngegevenWachtwoord,pasPlaats);

		if (IngegevenWachtwoord[3] != '\0')
		{
			//als het ingegeven wachtwoord overeenkomt met het juiste wachtwoord
			if(strcmp(WACHTWOORD, IngegevenWachtwoord) == 0){
				{
					disable_enableAlarm(0);
					memset(IngegevenWachtwoord,'\0',strlen(IngegevenWachtwoord));
					pasPlaats = 0;
					kansen = 3;
					printf("\nWACHTWOORD CORRECT\n");
				}
			}
			else
			{
				kansen --;
				pasPlaats = 0;
				memset(IngegevenWachtwoord,'\0',strlen(IngegevenWachtwoord));
				printf("\nWACHTWOORD INCORRECT - %d KANSEN OVER\n", kansen);
			}

			if(kansen == 0)
			{
				printf("\nAlarm gaat af\n");
				kansen = 3;
				alarmGaatAf(1);
				memset(IngegevenWachtwoord,'\0',strlen(IngegevenWachtwoord));
			}
		}
	}
}
