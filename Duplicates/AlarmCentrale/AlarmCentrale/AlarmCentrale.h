/*
 * AlarmCentrale.h
 */
//#define WACHTWOORD "1234"
char WACHTWOORD[]="1234";

#define WACHT_PERIODE 15

#define NR_ZONES 3
#define NR_SENSOREN_PER_ZONE 5
#define NR_AANLIGGENDE_ZONES 2
#define NR_ZONES_IN_LINK 2

#define LED_ON		PORTB |= 0B10000000;
#define LED_OFF		PORTB &= 0B01111111;
#define LED_TOGGLE	PORTB ^= 0B10000000;

#define START_TIMER	TCCR1B |= (1 << CS12); //start timer aan 1/256 van de klokspeed
#define STOP_TIMER	TCCR1B &= 0B11111000;
#define CLEAR_TIMER	TCNT1 = 0;

#define LEEG '\0'

#define TX_BUFFER_SIZE 16
#define LEES_BUFFER_SIZE 16
#define BERICHT_LENGTE 12

typedef struct zone
{
	int id;
	int sensor[NR_SENSOREN_PER_ZONE];
};

typedef struct links
{
	struct zone aanliggend[NR_AANLIGGENDE_ZONES];
};

ISR(TIMER4_COMPA_vect);
ISR(TIMER1_COMPA_vect);
