#ifndef COMMITTEE_H
#define  COMMITTEE_H

// #include "TinyTimber.h"
// #include "sciTinyTimber.h"
// #include "canTinyTimber.h"
// #include "sioTinyTimber.h"
// #include <stdlib.h>
// #include <stdio.h>
#include "globaldef.h"

#define INIT -1
#define SLAVE 1
#define MASTER 0
#define WAITING 2
#define COMPETE 3


typedef struct{
    Object super;
   	int boardNum; // init with 1
	int myRank; // init by user
	int leaderRank; // init with -1
    int mode;  //init with INIT
}Committee;

void committee_recv(Committee *,  int);
void send_Detecting_msg(Committee *, int);
// void send_Detecting_ack_msg(Committee *, int);
void send_BoardNum_msg(Committee *,int);
void send_Reset_msg(Committee *, int);
void send_GetLeadership_msg(Committee *,int);
void send_DeclareLeader_msg(Committee *,int);
void send_ResponseLeadership_msg (Committee *,int);
void I_to_W (Committee *,int);
void initBoardNum(Committee *,int);
void initMode(Committee *,int);
#endif