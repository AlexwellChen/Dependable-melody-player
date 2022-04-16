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
    Time send_time;
    int monitor_flag;
} Watchdog;

void watchdog_recv(Watchdog *, int );
void initWatchdog(Watchdog *,int);
void monitor(Watchdog *, int);
void check(Watchdog *, int);
int getMonitorFlag(Watchdog *, int);
void setMonitorFlag(Watchdog *, int);
//void send_ResponseWatchdog_msg(Watchdog *, int);
int isMasterExist(Watchdog *, int);
void send_F1_msg(Watchdog *, int);
void send_F2_msg(Watchdog *, int);
void send_Recovery_msg(Watchdog *, int);

#endif