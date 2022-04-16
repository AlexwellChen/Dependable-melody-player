#ifndef WATCHDOG_H
#define WATCHDOG_H

#include "globaldef.h"

#define DEACTIVE -2
#define ACTIVE 6
#define SNOOP_INTERVAL 100
typedef struct
{
	Object super;
	int networkState[3];
    Timer timer;
    TIme send_time;
} Watchdog;

void watchdog_recv(Watchdog *, int );
//void initWatchdog(Watchdog *,int);
void monitor(Watchdog *, int);
void check(Watchdog * self, int unused);
//void send_ResponseWatchdog_msg(Watchdog *, int);

#endif