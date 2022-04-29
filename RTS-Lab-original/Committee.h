#ifndef COMMITTEE_H
#define COMMITTEE_H

#include "globaldef.h"

#define INIT -1
#define SLAVE 1
#define MASTER 0
#define WAITING 2
#define COMPETE 3
#define FAILURE 4
#define F_1 5
#define F_2 6
#define F_3 7

typedef struct
{
	Object super;
	int boardNum;	// init with 1
	int myRank;		// init by user
	int leaderRank; // init with -1
	int mode;		// init with INIT
	int isLeader;
	int watchdogCnt;
} Committee;

void committee_recv(Committee *, int);
void send_Detecting_msg(Committee *, int);
// void send_Detecting_ack_msg(Committee *, int);
void send_BoardNum_msg(Committee *, int);
void send_Reset_msg(Committee *, int);
void send_GetLeadership_msg(Committee *, int);
void send_DeclareLeader_msg(Committee *, int);
void send_ResponseLeadership_msg(Committee *, int);
void IorS_to_W(Committee *, int);
void IorS_to_M(Committee*, int);
void initBoardNum(Committee *, int);
void initMode(Committee *, int);
void change_StateAfterCompete(Committee *, int);
void compete(Committee *, int);
void newCompete(Committee *, int );
int read_state(Committee *, int);
int getMyRank(Committee *, int);
int getLeaderRank(Committee *, int);
int getBoardNum(Committee *, int);
void setBoardNum(Committee *, int);
void enter_Failure(Committee *, int);
void exit_Failuremode(Committee *, int);
void changeLeaderRank(Committee *, int);
void committeeDebugOutput(Committee *, int);
void setMode(Committee *, int);
void D_to_F1(Committee *, int);
void D_to_F2(Committee *, int);
void D_to_F3(Committee *, int);
void F1_to_S(Committee *, int);
void F2_to_S(Committee *, int);
void F3_to_S(Committee *, int);
#endif