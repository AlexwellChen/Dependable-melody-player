#include "Committee.h"
#include "Watchdog.h"

extern App app;
extern Sound generator;
extern Controller controller;
extern Serial sci0;
extern SysIO sio0;
extern Can can0;
extern Watchdog watchdog;

Committee committee = {initObject(), 1, 1, -1, INIT, 1};

void committee_recv(Committee *self, int addr)
{
    CANMsg msg = *(CANMsg *)addr;
    char strbuff[100];
	if(msg.msgId != 119){
		snprintf(strbuff, 100, "Committe MSGID: %d\n", msg.msgId);
		SCI_WRITE(&sci0, strbuff);
	}
    int note;
    switch (self->mode)
    {
    case INIT:
        switch (msg.msgId)
        {
        case 122:
        {
            // For initBoardNum function
            SCI_WRITE(&sci0, "----------------Recv msgId 122---------------------\n");
            if (msg.nodeId != self->myRank)
                self->boardNum++;
            break;
        }
        case 123:
        {
            // For initMode function
            self->mode = SLAVE;
            // ASYNC(&app, setMode, SLAVE);
            // TODO: SYNC(initWatchdog)
            ASYNC(&watchdog, monitor, 0);
            self->leaderRank = msg.nodeId;
        }
        case 126:
        {
            // For initBoardNum function
            SCI_WRITE(&sci0, "----------------Recv msgId 126---------------------\n");
            int boardsNum_recv = atoi(msg.buff);
            if (boardsNum_recv > self->boardNum)
            {
                self->boardNum = boardsNum_recv;
            }
            // No need to print this line because 126 will be recieved three times
            // SCI_WRITE(&sci0,"Ready for getting leadership!\n");
            break;
        }
        }
        break;
    case MASTER:
        switch (msg.msgId)
        {
        case 127:
            self->mode = INIT;
            SCI_WRITE(&sci0, "Leadership Void\n");
            ASYNC(&app, setMode, INIT);
            break;
        }
        break;
    case SLAVE:

        switch (msg.msgId)
        {
        case 119:
            note = atoi(msg.buff);
            SCI_WRITE(&sci0, note);
            SCI_WRITE(&sci0, "\n");
            if (self->boardNum == 2 && note % 2 == 1)
            {
                SYNC(&controller, change_note, note);
                SYNC(&generator, set_turn, 1);
            }
            else if (self->boardNum == 3 && note % 3 == self->myRank)
            {
                SYNC(&controller, change_note, note);
                SYNC(&generator, set_turn, 1);
            }
            else
            {
                SYNC(&generator, set_turn, 0);
            }
            ASYNC(&controller, startSound, 0);
            break;
        }
        break;
    case WAITING:
        switch (msg.msgId)
        {
        case 127:
        {
            if (msg.nodeId > self->myRank)
            {
                // fail if has higher rank appears to compete
                self->isLeader = 0;
            }
            break;
        }
        }
        break;

    case FAILURE:
        break;
    }
}
int getMyRank(Committee *app, int unused)
{
    return app->myRank;
}
int getLeaderRank(Committee *app, int unused)
{
    return app->leaderRank;
}
int getBoardNum(Committee *app, int unused)
{
    return app->boardNum;
}
void send_BoardNum_msg(Committee *self, int arg)
{
    CANMsg msg;
    SCI_WRITE(&sci0, "--------------------send_BoardNum_msg-------------------------\n");
    char strbuff[100];
    snprintf(strbuff, 100, "BoardNum: %d \nMyRank: %d \n", self->boardNum, self->myRank);
    SCI_WRITE(&sci0, strbuff);
    msg.nodeId = self->myRank;
    msg.msgId = 126;
    char str_num[1];
    sprintf(str_num, "%d", abs(self->boardNum));
    msg.length = 1;
    msg.buff[0] = str_num[0];
    CAN_SEND(&can0, &msg);
}

void send_Detecting_msg(Committee *self, int num)
{
    CANMsg msg;
    SCI_WRITE(&sci0, "--------------------send_Detecting_msg-------------------------\n");
    char strbuff[100];
    snprintf(strbuff, 100, "BoardNum: %d\nLeaderRank: %d\nMyRank: %d\n", self->boardNum, self->leaderRank, self->myRank);
    SCI_WRITE(&sci0, strbuff);
    msg.nodeId = self->myRank;
    msg.msgId = 122;
    CAN_SEND(&can0, &msg);
    SCI_WRITE(&sci0,"CAN message send!\n");
}

// NOT USED ANYMORE
//  void send_Detecting_ack_msg(Committee* self,int num){
//  	CANMsg msg;
//  	SCI_WRITE(&sci0,"--------------------send_Detecting_ack-------------------------\n");
//  	char strbuff[100];
//  	snprintf(strbuff,100,"BoardNum: %d\nLeaderRank: %d\nMyRank: %d\n",self->boardNum,self->leaderRank,self->myRank);
//  	SCI_WRITE(&sci0,strbuff);
//  	msg.nodeId = self->leaderRank;
//  	msg.msgId = 121;
//  	CAN_SEND(&can0, &msg);
//  	SCI_WRITE(&sci0,"CAN message send!\n");
//  }

void send_Reset_msg(Committee *self, int arg)
{
    CANMsg msg;
    SCI_WRITE(&sci0, "--------------------send_Reset_msg-------------------------\n");
    // char strbuff[100];
    // snprintf(strbuff,100,"BoardNum: %d \nLeaderRank: %d \nMyRank: %d \n",self->boardNum,self->leaderRank,self->myRank);
    // SCI_WRITE(&sci0,strbuff);
    msg.nodeId = self->leaderRank;
    msg.msgId = 125;
    CAN_SEND(&can0, &msg);
}
void send_DeclareLeader_msg(Committee *self, int arg)
{
    CANMsg msg;
    SCI_WRITE(&sci0, "--------------------send_DeclareLeader_msg-------------------------\n");
    msg.nodeId = self->myRank;
    msg.msgId = 123;
    CAN_SEND(&can0, &msg);
}
void send_GetLeadership_msg(Committee *self, int arg)
{
    CANMsg msg;
    SCI_WRITE(&sci0, "--------------------Claim for Leadership-------------------------\n");
    msg.nodeId = self->myRank;
    msg.msgId = 127;
    CAN_SEND(&can0, &msg);
    self->mode = WAITING;
}

// NOT USED ANYMORE
//  void send_ResponseLeadership_msg (Committee *self ,int nodeId){
//      CANMsg msg;
//  	SCI_WRITE(&sci0,"--------------------Response for leadership compete-------------------------\n");
//  	msg.nodeId = self->myRank;
//  	msg.msgId = 124;
//      char str_num[1];
//     	sprintf(str_num,"%d", nodeId);
//     	msg.length = 1;
//      msg.buff[0] = str_num[0];
//      CAN_SEND(&can0, &msg);
//  }

void IorS_to_W(Committee *self, int arg)
{
    // Trying to get leadership from init mode or slave mode
    self->mode = WAITING;
}
void change_StateAfterCompete(Committee *self, int arg)
{
    if (self->isLeader)
    {
        self->mode = MASTER;
        self->leaderRank = self->myRank;
        ASYNC(&watchdog, monitor, self->myRank);
        ASYNC(self, send_DeclareLeader_msg, 0); // msgId 123
        if (self->leaderRank == self->myRank && self->mode == MASTER)
        {
            SCI_WRITE(&sci0, "Claimed Leadership!\n");
        }
    }
    else
    {
        self->mode = INIT;
    }
    // Reset before exit
    self->isLeader = 1;
}
void initBoardNum(Committee *self, int unused)
{
    ASYNC(&committee, send_Detecting_msg, 0); // msgId 122
    SCI_WRITE(&sci0, "send_Detecting_msg send!\n");
    // after two second, collect the board number and send to slaves
    AFTER(SEC(2), &committee, send_BoardNum_msg, 0); // msgId 126
}

void initMode(Committee *self, int unused)
{
    char strbuff[100];
    snprintf(strbuff, 100, "New boardNum: %d\n", self->boardNum);
    SCI_WRITE(&sci0, strbuff);
}
int read_state(Committee *self, int unused)
{
    return self->mode;
}

void compete(Committee *self, int unused)
{
    // Totoal 1 second, could it be shorter?
    ASYNC(&committee, IorS_to_W, 0);
    AFTER(MSEC(50), &committee, send_GetLeadership_msg, 0);
    AFTER(MSEC(100), &committee, change_StateAfterCompete, 0);
}

void setBoardNum(Committee *self, int arg)
{
    self->boardNum = arg;
}

void changeLeaderRank(Committee *self, int rank)
{
    self->leaderRank = rank;
}
void checkLeaderExist(Committee *self, int unused)
{
    // No leader in the system after recovery from F
    if (self->boardNum == 1)
    {
        ASYNC(self, compete, 0);
    }
    AFTER(MSEC(105), &controller, startSound, 0);
}
void exit_Failuremode(Committee *self, int arg)
{
    self->mode = SLAVE;
    // self->leaderRank = -1;
    ASYNC(&watchdog, send_Recovery_msg, 0);
    ASYNC(&app, compulsory_mute, 1);
    AFTER(MSEC(100), self, checkLeaderExist, 0);
}
void enter_Failure(Committee *self, int arg)
{
    char strbuff[100];
    if (arg == 1)
    {
        self->mode = F_1;
        ASYNC(&watchdog, send_F1_msg, 0);
        ASYNC(&app, compulsory_mute, 0);
        self->boardNum = 1;
    }
    else if (arg == 2)
    {
        self->mode = F_2;
        self->boardNum = 0;
        ASYNC(&watchdog, send_F2_msg, 0);
        ASYNC(&app, compulsory_mute, 0);
        AFTER(SEC(15), self, exit_Failuremode, 0);
    }
    else
    {
        snprintf(strbuff, 100, "Invalid failure mode number\n");
    }
}
void recover_Failure1mode(Committee *self, int arg)
{
    if (self->mode == F_1)
        self->mode = SLAVE;
}

void committeeDebugOutput(Committee * self, int arg){
    char strbuff[100];
    SCI_WRITE(&sci0, "Committee debug output:\n");
    snprintf(strbuff, 100, "boardNum: %d\nmyRank: %d\nleaderRank: %d\n", self->boardNum, self->myRank, self->leaderRank);
    SCI_WRITE(&sci0, strbuff);
    SCI_WRITE(&sci0, "mode: ");
    switch (self->mode)
    {
    case 0:
        /* code */
        SCI_WRITE(&sci0, "Master");
        break;
    case 1:
        /* code */
        SCI_WRITE(&sci0, "Slave");
        break;
    case -1:
        /* code */
        SCI_WRITE(&sci0, "Init");
        break;
    case 2:
        /* code */
        SCI_WRITE(&sci0, "Waiting");
        break;
    case 5:
        /* code */
        SCI_WRITE(&sci0, "Failure F1");
        break;
    case 6:
        /* code */
        SCI_WRITE(&sci0, "Failure F2");
        break;
    case 7:
        /* code */
        SCI_WRITE(&sci0, "Failure F3");
        break;
    default:
        break;
    }
    SCI_WRITE(&sci0, "\n");
}

void D_to_F1(Committee *self, int arg)
{
    self->mode = F_1;
}

void D_to_F2(Committee *self, int arg)
{
    self->mode = F_2;
}

void D_to_F3(Committee *self, int arg)
{
    self->mode = F_3;
}

void F1_to_S(Committee *self, int arg)
{
    self->mode = SLAVE;
}

void F2_to_S(Committee *self, int arg)
{
    self->mode = SLAVE;
}

void F3_to_S(Committee *self, int arg)
{
    self->mode = SLAVE;
}