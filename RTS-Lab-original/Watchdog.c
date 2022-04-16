#include "Watchdog.h"
#include "Committee.h"

extern App app;
extern Sound generator;
extern Controller controller;
extern Serial sci0;
extern SysIO sio0;
extern Can can0;
extern Committee committee;

Watchdog watchdog = {initObject(), {0}, initTimer(), 0, 1};

void watchdog_recv(Watchdog *self, int addr)
{
    CANMsg msg = *(CANMsg *)addr;
    char strbuff[100];
    snprintf(strbuff, 100, "Watchdog MSGID: %d\n", msg.msgId);
    SCI_WRITE(&sci0, strbuff);
    int mode = SYNC(&committee, read_state, 0);
    int leaderRank = SYNC(&committee, getLeaderRank, 0);
    int myRank = SYNC(&committee, getMyRank, 0);
    int boardNum = SYNC(&committee, getBoardNum, 0);
	Time now;
    switch (msg.msgId)
    {
    case 63:
        // update the state array (in a given time interval)
        now = T_SAMPLE(&self->timer);
        if ((now - self->send_time) < MSEC(SNOOP_INTERVAL))
        {
            if (msg.nodeId == leaderRank)
                self->networkState[msg.nodeId] = MASTER;
            else
                self->networkState[msg.nodeId] = SLAVE;
        }
        break;
    case 62: // Master Failure F1
        self->networkState[msg.nodeId] = F_1;
        ASYNC(&committee, setBoardNum, boardNum - 1);
        break;
    case 61: // Slave Failure F1
        self->networkState[msg.nodeId] = F_2;
        ASYNC(&committee, setBoardNum, boardNum - 1);
        break;
    case 60: // New member join
        ASYNC(self, setMonitorFlag, 0);
        ASYNC(&committee, setBoardNum, boardNum + 1);
        // Force Synchronization
        AFTER(MSEC((int)(1.5 * SNOOP_INTERVAL)), self, setMonitorFlag, 1);
        AFTER(MSEC(2 * SNOOP_INTERVAL), self, monitor, 0);
        break;
    }
}

void check(Watchdog *self, int unused)
{
    int boardNum = SYNC(&committee, getBoardNum, 0);
    int leaderRank = SYNC(&committee, getLeaderRank, 0);
    int cntDeactive = 0;
    for (int i = 0; i < 3; i++)
    {   
        if(self->networkState[i] == DEACTIVE){
            cntDeactive++;
            self->networkState[i] = F_3; //passive enter F3
            ASYNC(&committee, setBoardNum, boardNum - 1);
        }
    }
    if(cntDeactive == 2){
        // MDD or SDD means current board is out, which indicates ourself enter F3.
        ASYNC(&committee, D_to_F3, 0); // go into F3 mode;
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
    if(!isMasterExist){ // There is no Master in current network
        ASYNC(&committee, compete, 0);  
        // Is it possible get a FFF here? 
    }

    if (self->monitor_flag)
    {
        ASYNC(self, monitor, 0);
    }
}
void monitor(Watchdog *self, int unused)
{
    CANMsg msg;
    for (int i = 0; i < 3; i++)
    {
        // Keep failure states
        if (self->networkState[i] == MASTER || self->networkState[i] == SLAVE)
            self->networkState[i] = DEACTIVE;
    }
    int myRank = SYNC(&committee, getMyRank, 0);
    int leaderRank = SYNC(&committee, getLeaderRank, 0);
    if (myRank == leaderRank)
        self->networkState[myRank] = MASTER;
    else
        self->networkState[myRank] = SLAVE;

    msg.nodeId = myRank;
    msg.msgId = 63;
    CAN_SEND(&can0, &msg);
    Time now = T_SAMPLE(&self->timer);
    self->send_time = now;

    AFTER(MSEC(SNOOP_INTERVAL), self, check, 0);
}

void initWatchdog(Watchdog *self, int arg){
    /*
    *   由于monitor里的for循环需要保持failure的状态，
    *   所以还是需要initWatchdog来初始化networkState。
    *   在initMode后，执行monitor前
    */

}

int getMonitorFlag(Watchdog *self, int arg)
{
    return self->monitor_flag;
}
void setMonitorFlag(Watchdog *self, int flag)
{
    self->monitor_flag = flag;
}

int isMasterExist(Watchdog *self, int arg){
    for(int i = 0; i < 3; i++){
        if(self->networkState[i] == MASTER)
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