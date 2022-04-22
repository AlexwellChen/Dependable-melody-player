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

#define DAC_port ((volatile unsigned char *)0x4000741C)
typedef struct
{
	Object super;
	int count;
	char c[100];
	Time nums[3];
	int nums_count;
	int mode;
	//-1 is init, 0 is master, 1 is slave
	int press_mode;
	Timer timer;
	int bounce;
	int momentary;
	Time previous_time;
	int trigmode;
	int inteval;
	int boardNum;	// init with -1
	int myRank;		// init with -1
	int leaderRank; // init with -1
	int print_flag;
	int monitorFlag; // init with 1
} App;

typedef struct
{
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
 * volume: the current output volume for sound generator
 * pre_volume: keeps the previous volume before muted
 * deadline_enabled: the deadline enabled state, 0 for false(disabled) 1 for true(enabled)
 */
typedef struct
{
	Object super;
	int play;
	int flag;
	int volume;
	int prev_volume;
	int deadline_enabled;
	int gap;
	int period;
	int turn;
} Sound;

void reader(App *, int);
void receiver(App *, int);
void user_call_back(App *, int);
void three_history(App *, Time);
void send_key_msg(App *, int);
void send_bmp_msg(App *, int);
void send_note_msg(App *, int);
void initNetwork(App *, int);
void setMode(App *, int);

void change_note(Controller *, int);

void compulsory_mute (Sound *, int);
void mute(Sound *);
void volume_control(Sound *, int);
void pause(Sound *, int);
int getMute(Sound *, int);
void set_turn(Sound *, int);
void set_play(Sound*,int);
int judgePlay(Sound *self, int note);
void reset_gap(Sound*, int);
void gap(Sound*, int);
void change_period(Sound*,int);
void play(Sound*,int);

void startSound(Controller *, int);
int getBpm(Controller *, int);
void pause_c(Controller *, int);
void set_print_flag(Controller *, int);
void change_key(Controller *, int);
void change_bpm(Controller *, int);
void print_tempo(Controller *, int);
void replay (Controller*, int);
void passive_backup (Controller*, int);

#endif