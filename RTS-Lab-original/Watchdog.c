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

    switch (msg.msgId)
    {
    case 63:
        // update the state array (in a given time interval)
        Time now = T_SAMPLE(&self->timer);
        if ((now - self->send_time) < MSEC(SNOOP_INTERVAL))
        {
            if (msg.nodeId == leaderRank)
                self->networkState[msg.nodeId] = MASTER;
            else
                self->networkState[msg.nodeId] = SLAVE;
        }
        break;
    case 62: // Master Failure F1
        self->networkState[leaderRank] = F_1;
        break;
    case 61: // Slave Failure F1
        self->networkState[msg.nodeId] = F_1;
        break;
    case 60: // Master Failure F2
        self->networkState[leaderRank] = F_2;
        break;
    case 59: // Slave Failure F2
        self->networkState[msg.nodeId] = F_2;
        break;
    case 58: // New member join
        ASYNC(self, setMonitorFlag, 0);
        // TODO: add boardNum
        // Force Synchronization
        AFTER(MSEC(10), self, monitor, 0);
        break;
    }
}
void check(Watchdog *self, int unused)
{
    int leaderRank = SYNC(&committee, getLeaderRank, 0);
    for (int i = 0; i < 3; i++)
    {
        self->networkState[i] = F_3;
    }
    // TODO: add stateHandler() of networkState
    /*
     * We have total five combinations: 
     * MSS(normal)
     * MSF(Slave failure)
     * MFF(Slave failure)
     * FSS(Master failure)
     * FSF(Master failure)
     * Disregard the order
     */
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
void send_ResponseWatchdog_msg(Watchdog *self, int unused)
{
    CANMsg msg;
    int myRank = SYNC(&committee, getMyRank, 0);
    msg.nodeId = myRank;
    msg.msgId = 62;
    CAN_SEND(&can0, &msg);
}

int getMonitorFlag(Watchdog *self, int arg)
{
    return self->monitor_flag;
}
void setMonitorFlag(Watchdog *self, int flag)
{
    self->monitor_flag = flag;
}