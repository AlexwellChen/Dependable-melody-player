#ifndef GLOBALDEF_H
#define GLOBALDEF_H
#include "TinyTimber.h"
#include "sciTinyTimber.h"
#include "canTinyTimber.h"
#include "sioTinyTimber.h"
#include <stdlib.h>
#include <stdio.h>
/*
0 2 4 0
0 2 4 0
4 5 7 -
4 5 7 -
*/
// float frequencies[32] = {
// 	440.00, 493.88, 554.37, 440.00, //0240
// 	440.00, 493.88, 554.37, 440.00, //0240
// 	554.37, 587.33, 659.25, 554.37, //4574
// 	587.33, 659.25, 659.25, 739.99, //5779
// 	659.25, 587.33, 554.37, 440.00, //7540
// 	659.25, 739.99, 659.25, 587.33, //7975
// 	554.37, 440.00, 440.00, 329.63, //400-5
// 	440.00, 440.00, 329.63, 440.00 //00-50
// };

#define DAC_port ((volatile unsigned char*) 0x4000741C)
typedef struct {
    Object super;
    int count;
    char c[100];
    Time nums[3];
    int nums_count ;
	int mode;
	//-1 is init, 0 is master, 1 is slave
	int press_mode;
	Timer timer;
	int bounce;
	int momentary;
	Time previous_time;
	int trigmode;
	int inteval;
	int boardNum; // init with -1
	int myRank; // init with -1
	int leaderRank; // init with -1
	int print_flag;
} App;

typedef struct {
    Object super;
    int play;
	int key;
	int note;
    int bpm;
	int change_bpm_flag;
	int print_flag;
} Controller;

/* Application class for sound generator
*  flag: whether to write 0 or 1 to the DAC port
* volumn: the current output volumn for sound generator
* pre_volumn: keeps the previous volumn before muted
* deadline_enabled: the deadline enabled state, 0 for false(disabled) 1 for true(enabled)
*/
typedef struct {
    Object super;
	int play;
	int flag;
	int volumn;
	int prev_volumn;
	int  deadline_enabled;
    int gap;
    int period;
}Sound;

void reader(App*, int);
void receiver(App*, int);
void user_call_back(App*, int);
int getMyRank(App*, int);
int getLeaderRank(App*, int);
int getBoardNum(App*, int);
void three_history(App *,Time);


void startSound(Controller* , int);
void mute (Sound* );
void volume_control (Sound* , int );
void pause(Sound *, int );
void pause_c(Controller *, int );
void set_print_flag(Controller*, int);
void change_key(Controller *, int );
void change_bpm(Controller *, int );
void print_tempo(Controller *, int );


#endif