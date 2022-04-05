#ifndef COMMITTEE_H
#define  COMMITTEE_H

#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include "sioTinyTimber.h"
#include <stdlib.h>
#include <stdio.h>
#include "globaldef.h"

#define INIT -1
#define SLAVE 1
#define MASTER 0
#define WAITING 2
#define COMPETE 3


typedef struct{
    Object super;
   	int boardNum; // init with -1
	int myRank; // init with -1
	int leaderRank; // init with -1
    int mode;  //init with -1 
}Committee;

Committee committee = { initObject(), -1,0 , -1,INIT};

void committee_recv(){

}
#endif