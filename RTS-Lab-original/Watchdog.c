#include "Watchdog.h"
#include "Committee.h"

#define __CAN_LOOPBACK 1

extern App app;
extern Sound generator;
extern Controller controller;
extern Serial sci0;
extern SysIO sio0;
extern Can can0;
extern Committee committee;

Watchdog watchdog = {initObject(), {-2, -2, -2}, {-2,-2,-2}, initTimer(), 0, 1, 0, 0};

void watchdog_recv(Watchdog *self, int addr)
{
    CANMsg msg = *(CANMsg *)addr;
    char strbuff[100];
    snprintf(strbuff, 100, "Watchdog MSGID: %d\n", msg.msgId);
    if (msg.msgId != 119)
    {
        SCI_WRITE(&sci0, strbuff);
    }
    int mode = SYNC(&committee, read_state, 0);
    int leaderRank = SYNC(&committee, getLeaderRank, 0);
    int myRank = SYNC(&committee, getMyRank, 0);
    int boardNum = SYNC(&committee, getBoardNum, 0);
    int boardsNum_recv;
    switch (msg.msgId)
    {
    case 64:
        self->networkState[msg.nodeId] = MASTER;
        self->networkStateforCheck[msg.nodeId] = MASTER;
        break;
    case 63:
        self->networkState[msg.nodeId] = SLAVE;
        self->networkStateforCheck[msg.nodeId] = SLAVE;
        break;
    case 62: // Failure F2
        if (mode == F_3)
        {
            // Used for F_3 enter |F_3|F_2|F_1|
            self->networkState[myRank] = SLAVE;
        }
        self->networkState[msg.nodeId] = F_2;

        break;
    case 61: // Failure F1

        if (mode == F_3)
        {
            // Used for F_3 enter |F_3|F_2|F_1|
            self->networkState[myRank] = SLAVE;
            // Set to Slave, in check stage the boardNum will change to 1,
            // and then the committee->mode will change to Slave.
        }
        self->networkState[msg.nodeId] = F_1;

        break;
    case 60: // New member join
        ASYNC(&committee, setBoardNum, boardNum + 1);
        ASYNC(self, send_Recovery_ack, 0);
        break;
    case 59:
        boardsNum_recv = atoi(msg.buff);
        ASYNC(&committee, setBoardNum, boardsNum_recv);
        ASYNC(&committee, changeLeaderRank, msg.nodeId);
    }
}

void check(Watchdog *self, int unused)
{
    int leaderRank = SYNC(&committee, getLeaderRank, 0);
    int cntDeactive = 0;
    int boardNum = 0;
    int slaveNum = 0;
    int masterNum = 0;
    int myMode = SYNC(&committee, read_state, 0);
    int previous_Bnum = SYNC(&committee, getBoardNum, 0);
    char strbuff[50];
    for (int i = 0; i < 3; i++)
    {
        if (myMode != F_1 && myMode != F_2)
        {
            self->networkStateforCheck[i] = self->networkState[i];
        }

        if (self->networkState[i] == DEACTIVE)
        {
            cntDeactive++;
            self->networkState[i] = F_3; // passive enter F3
            self->networkStateforCheck[i] = F_3;
            snprintf(strbuff, 100, "board %d out\n", i);
            SCI_WRITE(&sci0, strbuff);
            // ASYNC(&committee, setBoardNum, boardNum - 1);
        }
        if (self->networkState[i] == MASTER)
        {
            boardNum++;
            masterNum++;
        }
        if (self->networkState[i] == SLAVE)
        {
            boardNum++;
            slaveNum++;
        }
        
    }
    self->masterNum = masterNum;
    self->slaveNum = slaveNum;

    if (boardNum < previous_Bnum)
    {
        ASYNC(&controller, passive_backup, 0);
    }

    ASYNC(&committee, setBoardNum, boardNum);

    if (masterNum > 1)
    {
        snprintf(strbuff, 100, "Compete at Master num > 1\n");
        SCI_WRITE(&sci0, strbuff);
        ASYNC(&committee, newCompete, 0);
    }

    if (myMode == F_3 && boardNum > 0)
    {
        // recover, change from F_3 to slave
        ASYNC(&committee, F3_to_S, 0);
    }
    /*
     * We have total five combinations:
     * MSS(normal)
     * MSF(Slave failure)
     * MFF(Slave failure)
     * FSS(Master failure)
     * FSF(Master failure)
     * Disregard the order
     */
    if (masterNum == 0)
    { // There is no Master in current network
        if (boardNum == 1 && myMode == SLAVE)
        {
            // ASYNC(&committee, D_to_F3, 0);
            if (self->networkState[leaderRank] == F_1 || self->networkState[leaderRank] == F_2)
            {
                ASYNC(&committee, newCompete, 0);
                snprintf(strbuff, 100, "Compete at Master in F1 and F2\n");
                SCI_WRITE(&sci0, strbuff);
            }else{
                ASYNC(&controller, set_play, 0);
            }
        }
        else if (myMode != F_1 && myMode != F_2 && myMode != F_3)
        {
            snprintf(strbuff, 100, "Compete at Master out, 2 slaves\n");
            SCI_WRITE(&sci0, strbuff);
            ASYNC(&committee, newCompete, 0);
        }
        // Is it possible get a FFF here?
    }
    AFTER(MSEC(SNOOP_INTERVAL), self, check, 0);
    for (int i = 0; i < 3; i++)
    {
        self->networkState[i] = DEACTIVE;
    }
    // self->networkState[SYNC(&committee, getMyRank, 0)] = myMode;
    // if (myMode == F_1 || myMode == F_2)
    // {
    //     ASYNC(self, monitor, 0);
    // }
}

void monitor(Watchdog *self, int unused)
{
    CANMsg msg;
    int myRank = SYNC(&committee, getMyRank, 0);
    int myMode = SYNC(&committee, read_state, 0);
    self->networkState[myRank] = myMode;

    msg.nodeId = myRank;
    switch (myMode)
    {
    case MASTER:
        msg.msgId = 64;
        break;
    case SLAVE:
        msg.msgId = 63;
        break;
    case F_3:
        msg.msgId = 63;
        break;
    case F_2:
        msg.msgId = 62;
        break;
    case F_1:
        msg.msgId = 61;
        break;
    }
    if (myMode != F_1 && myMode != F_2)
    {
        CAN_SEND(&can0, &msg);
    }
    AFTER(MSEC(SNOOP_INTERVAL), self, monitor, 0);
}

int getMonitorFlag(Watchdog *self, int arg)
{
    return self->monitor_flag;
}
void setMonitorFlag(Watchdog *self, int flag)
{
    self->monitor_flag = flag;
}

int isMasterExist(Watchdog *self, int arg)
{
    for (int i = 0; i < 3; i++)
    {
        if (self->networkState[i] == MASTER)
            return 1;
    }
    return 0;
}

void send_F1_msg(Watchdog *self, int unused)
{
    CANMsg msg;
    int myRank = SYNC(&committee, getMyRank, 0);
    msg.nodeId = myRank;
    msg.msgId = 62;
    CAN_SEND(&can0, &msg);
}

void send_F2_msg(Watchdog *self, int unused)
{
    CANMsg msg;
    int myRank = SYNC(&committee, getMyRank, 0);
    msg.nodeId = myRank;
    msg.msgId = 61;
    CAN_SEND(&can0, &msg);
}

void send_Recovery_msg(Watchdog *self, int unused)
{
    CANMsg msg;
    int myRank = SYNC(&committee, getMyRank, 0);
    msg.nodeId = myRank;
    msg.msgId = 60;
    CAN_SEND(&can0, &msg);
}
void send_Recovery_ack(Watchdog *self, int unused)
{
    CANMsg msg;
    int myMode = SYNC(&committee, read_state, 0);
    int myRank = SYNC(&committee, getMyRank, 0);
    if (myMode == MASTER)
    {
        msg.nodeId = myRank;
        msg.msgId = 59;
        int boardNum = SYNC(&committee, getBoardNum, 0);
        char str_num[1];
        sprintf(str_num, "%d", abs(boardNum));
        msg.length = 1;
        msg.buff[0] = str_num[0];
        CAN_SEND(&can0, &msg);
    }
    else
    {
        return;
    }
}

void updateStoM(Watchdog *self, int arg){
    int myRank = SYNC(&committee, getMyRank, 0);
    self->networkState[myRank] = arg;
    self->networkStateforCheck[myRank] = arg;
}

void updateMasterInNetworkState(Watchdog *self, int arg){
    self->networkState[arg] = MASTER;
    self->networkStateforCheck[arg] = MASTER;
}

void updateSlaveInNetworkState(Watchdog *self, int arg){
    self->networkState[arg] = SLAVE;
    self->networkStateforCheck[arg] = SLAVE;
}


void watchdogDebugOutput(Watchdog *self, int arg)
{
    char strbuff[100];
    snprintf(strbuff, 100, "\nWatchdog Debug Output\n");
    SCI_WRITE(&sci0, strbuff);
    for (int i = 0; i < 3; i++)
    {
        snprintf(strbuff, 100, "board %d mode: ", i);
        SCI_WRITE(&sci0, strbuff);
        switch (self->networkStateforCheck[i])
        {
        case 0:
            /* code */
            SCI_WRITE(&sci0, "Master\n");
            break;
        case 1:
            /* code */
            SCI_WRITE(&sci0, "Slave\n");
            break;
        case -1:
            /* code */
            SCI_WRITE(&sci0, "Init\n");
            break;
        case 2:
            /* code */
            SCI_WRITE(&sci0, "Waiting\n");
            break;
        case 5:
            /* code */
            SCI_WRITE(&sci0, "Failure\n");
            break;
        case 6:
            /* code */
            SCI_WRITE(&sci0, "Failure\n");
            break;
        case 7:
            /* code */
            SCI_WRITE(&sci0, "Failure\n");
            break;
        case -2:
            SCI_WRITE(&sci0, "Deactive\n");
            break;
        default:
            break;
        }
    }
    snprintf(strbuff, 100, "Master num: %d\nSlave num: %d\n", self->masterNum, self->slaveNum);
    SCI_WRITE(&sci0, strbuff);
}