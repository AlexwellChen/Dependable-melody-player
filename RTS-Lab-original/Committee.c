#include "Committee.h"

void committee_recv(Committee * self,  int addr){
    CANMsg msg = *(CANMsg *) addr;
    switch(self->mode){
        case INIT:
            switch(msg.msgId){

            }
        case MASTER:
        case SLAVE:
        case WAITING:
        case COMPETE:
    }
}
