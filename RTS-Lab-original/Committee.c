#include "Committee.h"

extern App app;
extern Sound generator;
extern Controller controller;
extern Serial sci0;
extern SysIO sio0;
extern Can can0;

void committee_recv(Committee * self,  int addr){
    CANMsg msg = *(CANMsg *) addr;
   char strbuff[100];
	snprintf(strbuff,100,"Committe MSGID: %d\n",msg.msgId);
	SCI_WRITE(&sci0,strbuff);
    switch(self->mode){
        case INIT:
            switch(msg.msgId){

            }
            break;
        case MASTER:
            break;
        case SLAVE:
            break;
        case WAITING:
             break;  
        case COMPETE:
            break;
    }
}
