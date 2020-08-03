/*
 * AlarmCentrale.h
 * Author : Pieter-Jan Steeman
 */
#define WACHTWOORD "1234"

#define GEBRUIK_ZONES 1

#define NR_ZONES 3
#define NR_SENSOREN_PER_ZONE 5
#define NR_AANLIGGENDE_ZONES 2
#define NR_ZONES_IN_LINK 2

typedef struct zone
{
	int id;
	int sensor[NR_SENSOREN_PER_ZONE];
};

typedef struct links
{
	struct zone aanliggend[NR_AANLIGGENDE_ZONES];
};
