/*User Guide

Compile and upload the .s19 file to the experiment board, type "go" to start the execution.
 
!! The default mode is paused, press 'p' to play after 'go'


 VOLUME CONTROL
 
 - For increasing volume press "q"
 - For decreasing press "a"
 
 MUTE CONTROL
 
 - For mute and unmute press "m"
 
 LOAD CONTROL
 
 - For increasing background load press "w"
 - For decreasing background load press "s"
 
 DEADLINE CONTROL
  
 - For enable and disable deadline press "d"
 
 PLAY
  
 - To pause and play press "p"
 
 CHANGE KEY

 - When a number pressed with "k" at the end new key value is set
 
 CHANGE BPM

 - When a number pressed with "b" at the end new bpm value is set
 
 PRESS HOLD FUNCTIONALITY
 
 - When user button is pressed for  at least 1 seconds, it enters press hold mode
 
 RESET BPM
 
  - When user button is pressed for  at least 2 seconds, it resets bpm to 120
 
 CHANGE TEMPO FROM USER BUTTON
 
 - In the serial momentary presses with user button new bpm is set.

*/
// #include "TinyTimber.h"
// #include "sciTinyTimber.h"
// #include "canTinyTimber.h"
// #include "sioTinyTimber.h"
// #include <stdlib.h>
// #include <stdio.h>
//#include "globaldef.h"
#include "Committee.h"

const int myIndex[32]={
	0,2,4,0,
	0,2,4,0,
	4,5,7,
	4,5,7,
	7,9,7,5,4,0,
	7,9,7,5,4,0,
	0,-5,0,
	0,-5,0	
};
const float periods[25] = {0.00202478, 0.0019111,  0.00180388, 0.00170265, 0.00160705, 0.00151685,
 0.00143172, 0.00135139, 0.00127551, 0.00120395, 0.00113636, 0.00107259,
 0.00101239, 0.00095557, 0.00090192, 0.00085131, 0.00080354, 0.00075844,
 0.00071586, 0.00067568, 0.00063776, 0.00060197, 0.00056818, 0.00053629,
 0.00050619};

const int beats[32] = {2,2,2,2,2,
    2,2,2,2,2,
    4,2,2,4,1,
    1,1,1,2,2,
    1,1,1,1,2,
    2,2,2,4,2,
    2,4};


App app = { initObject(), 0, 'X', {0},0,-1,0,initTimer(),0,0,0,0,0,-1,-1,-1,-1,0,1};
Sound generator = { initObject(), 0,0 , 5,0,1,0,0};
Controller controller =  { initObject(),0,0,0,120,0,0};
Serial sci0 = initSerial(SCI_PORT0, &app, reader);
SysIO sio0 = initSysIO(SIO_PORT0, &app,user_call_back);
Can can0 = initCan(CAN_PORT0, &app, receiver);
extern Committee committee;

void check_hold(App *self, int unused){
	int state = SIO_READ(&sio0);
	Time now = T_SAMPLE(&self->timer) ;
	if((now-self->previous_time)>=SEC(1)&&(state==0))
	//press state: 1 for released, 0 for pressed
	SCI_WRITE(&sci0,"Entered press-hold mode\n");	
}

void three_history(App *self,Time num){
    Time sum = 0;
    
    if ( self -> nums_count < 3){
       self -> nums[self->nums_count] = num;
       self -> nums_count ++;
       
    }else{
        self-> nums[0] = num;
        //made swap if more than 3 elements added
        Time temp = 0;
        temp = self -> nums[0];
        self-> nums[0] = self -> nums[1];
        self -> nums[1] = self -> nums[2];
        self -> nums[2] = temp;
    }
	//check if differenca is smaller than 100msec
	if(self->nums_count==3){
		if(abs(self->nums[0]-self->nums[1])<MSEC(100) &&
		abs(self->nums[0]-self->nums[2])<MSEC(100) &&
		abs(self->nums[2]-self->nums[1])<MSEC(100) ){
		//calculate avg,change bpm
		Time avg = (self->nums[0]+self->nums[1]+self->nums[2])/3;
		float new_time = SEC_OF(avg) + (float)(MSEC_OF(avg))/1000.0;
		int new_bpm = (int)60.0/new_time;
	 	char output[200];
	 	snprintf(output,200,"New bpm is %d\n", new_bpm);
    		SCI_WRITE(&sci0,output);
			
			SYNC(&controller,change_bpm,new_bpm);
		}else{
			//do nothing
		}
	}

}
void user_call_back(App *self, int unused){
	
	if(self->trigmode == 0){
		Time start =T_SAMPLE(&self->timer);
		Time inteval = start - self->previous_time;
		self->inteval = inteval;
		self->previous_time = start;
		self->trigmode = 1;
		SIO_TRIG(&sio0,1);
		SEND(SEC(1),MSEC(50),self,check_hold,0);
	}else{
		Time end =T_SAMPLE(&self->timer);
		Time diff = end - self->previous_time;
		
		if(diff<SEC(1)){
			
			diff = self->inteval;
			if(diff<MSEC(100)){
				SCI_WRITE(&sci0,"bounce \n");
				self->trigmode = 0;
				SIO_TRIG(&sio0,0);
				return ;
			}
			long usec = USEC_OF(diff);
			long msec = MSEC_OF(diff);
			long sec = SEC_OF(diff);
			char WCET[200];
			snprintf(WCET,200,"Momentary press interval is %ld sec, %ld msec, %ld usec  \n",sec,msec,usec);
			ASYNC(self,three_history,diff);
			SCI_WRITE(&sci0,WCET);
		}else{
			
			long usec = USEC_OF(diff);
			long msec = MSEC_OF(diff);
			long sec = SEC_OF(diff);
			char WCET[200];
			snprintf(WCET,200,"Hold time is %ld sec, %ld msec, %ld usec  \n",sec,msec,usec);
			SCI_WRITE(&sci0,WCET);
			if(diff>SEC(5)){
				//slave get leadership
				ASYNC(self, send_GetLeadership_msg, 0);
			}else
			if(diff>SEC(2)){
				SCI_WRITE(&sci0,"Reset bpm to 120\n");
				// int bpm = 120;
				// SYNC(&controller,change_bpm,bpm);
				if(self->mode ==0){
					//check if leader
					SCI_WRITE(&sci0,"Reseting BPM and key from master\n");
					ASYNC(self, send_Reset_msg, 0);
					SYNC(&controller, change_bpm, 120);
					SYNC(&controller, change_key, 0);
				}
			}
			//T_RESET(&self->timer);
		}
		self->trigmode = 0;
		SIO_TRIG(&sio0,0);
		
	}
}

void initNetwork(App* self, int unused){
	//Self init always have one board
	self->boardNum = 1;
	self->mode = -1;
	self->leaderRank = -1;

	//Initialize our rank as 0
	self->myRank = 0;
}


int getMyRank(App* app, int unused){
	return app->myRank;
}
int getLeaderRank(App* app, int unused){
	return app->leaderRank;
}
int getBoardNum(App* app, int unused){
	return app->boardNum;
}
int getMute(Sound* self, int unused){
	return self->volume;
}

void monitor(App* self, int unused){
	int volume = SYNC(&generator, getMute, 0);
	int bpm = SYNC(&controller, getBpm, 0);
	char strbuff[20];
	if(self->mode == MASTER && self->monitorFlag == 1){
		snprintf(strbuff,100,"Current bpm: %d\n", bpm);
		SCI_WRITE(&sci0,strbuff);
	}else if(self->mode == SLAVE && volume == 0 && self->monitorFlag == 1){
		SCI_WRITE(&sci0,"MUTED\n");
	}
	AFTER(SEC(10), self, monitor, 0);
}

/* CAN protocol
 * msgId->1: increase the volume
 * msgId->2: decrease the volume
 * msgId->3: mute the melody
 * msgId->4: pause the melody
 * msgId->5: change the positive key msg.buff = new value (buffer size = 1)
 * msgId->6: change the negative key msg.buff = new value (buffer size = 1)
 * msgId->7: change the bpm msg.buff = new value (buffer size = 3)
 * msgId->8: reset the key and tempo
 * msgId->121: Response to detect member.
 * msgId-122: Detect member in the network.
 * msgId->123: Declare Leadership.
 * msgId->124: response to leader requirement.
 * msgId->125: reset the bpm to 120 and key to 0, nodeid is leader’s rank.
 * msgId->126: send board number in current network
 * msgId->127: Claim for leadership
 */
void receiver(App* self, int unused)
{	
	SCI_WRITE(&sci0,"--------------------receiver-------------------------\n");
    CANMsg msg;
    CAN_RECEIVE(&can0, &msg);
	SCI_WRITE(&sci0, "Can msg received: \n");
	char strbuff[100];
	snprintf(strbuff,100,"ID: %d\n",msg.msgId);
	SCI_WRITE(&sci0,msg.msgId);
	if(msg.msgId>100){
		ASYNC(&committee,committee_recv,&msg);
	}
	int num = 0;
	if(self->mode){
		switch(msg.msgId){
			case 1: 
				ASYNC(&generator,volume_control,1);
				break;
			case 2: 
				ASYNC(&generator,volume_control,0);
				break;
			case 3: 
				ASYNC(&generator,mute,0);
				break;
			case 4: 
				SYNC(&generator,pause,0);
				SYNC(&controller,pause_c,0);
				break;
			case 5:
				//positive key
				num = atoi(msg.buff);
				SYNC(&controller,change_key,num);
				break;
			case 6:
				//negative key
				num = atoi(msg.buff);
				SYNC(&controller,change_key,-num);
				break;
			case 7:
				num = atoi(msg.buff);
				SYNC(&controller,change_bpm,num);
				break;
			case 8:
				SYNC(&controller, change_bpm, 120);
				SYNC(&controller, change_key, 0);
				break;
			case 9:
				break;
			
			default: break;
		}
	}
}


void volume_control (Sound* self, int inc){
	if(inc==1&&self->volume<20)
		self->volume ++;
	else if (inc==0&&self->volume>1)
		self->volume --;
}	

void deadline_control_sound(Sound* self, int arg){
	if(self->deadline_enabled==0){
		self->deadline_enabled = 1;
	}else{
		self->deadline_enabled = 0;
	}
}

void print_mute_state(App *self, int arg){
	int volume = SYNC(&generator,getMute,0);
	if(volume==0&&self->print_flag&&self->mode==1){
		SCI_WRITE(&sci0, "Board is muted\n");
		AFTER(SEC(10),&app, print_mute_state,0);
	}else return;
}

void mute (Sound* self){
	if(self->volume == 0){
		self->volume = self->prev_volume;
		//SCI_WRITE(&sci0, "Board is unmuted\n");
	}else{
		self->prev_volume = self->volume;
		self->volume = 0;
		//SCI_WRITE(&sci0, "Board is muted\n");
		ASYNC(&app, print_mute_state,0);
	}	
}

/* 1kHZ : 500us  
769HZ: 650us
537HZ: 931us
*/
void gap(Sound*self, int arg){
	self->gap = 1;
}
void play(Sound* self, int arg){
    
    self->flag = !self->flag;
    if(self->gap){
        *DAC_port = 0x00;
    }else{
        if(self->flag){
            *DAC_port = self->volume;
        }else{
            *DAC_port = 0x00;
        }
    }
	if(self->play){
		if(self->deadline_enabled){
				SEND(USEC(self->period),USEC(self->period),self,play,0);
			}else{
				AFTER(USEC(self->period),self,play,0);
			}
	}else{
		return ;
	}  
}
void change_period(Sound* self, int arg){
	self->period = arg;
}
void reset_gap(Sound* self, int arg){
	self->gap = 0;
}

int getVolume(Sound* self, int arg){
	return self->volume;
}

void toggle_led(Controller* self, int arg){
	if(self->play ==0 || self->bpm!=arg) return;
	SIO_TOGGLE(&sio0);
	
	float interval = 60.0/(float)self->bpm;
	SEND(MSEC(500*interval),MSEC(250*interval),self,toggle_led,self->bpm);
}
void startSound(Controller* self, int arg){
	ASYNC(&app, send_note_msg, self->note);// Send current noteId before playing this note.
	if(self->play==0) return;
	int boardNum = SYNC(&app, getBoardNum, 0);
	int myRank = SYNC(&app, getMyRank, 0);
	SYNC(&generator,reset_gap,0);
	
	int offset = self->key + 5+5;
    int period = periods[myIndex[self->note]+offset]*1000000;
	SYNC(&generator,change_period,period);
	
    int tempo = beats[self->note];
//	if(tempo>=2) SIO_WRITE(&sio0,0);
   
	
    float interval = 60.0/(float)self->bpm;
	if(self->bpm!=arg) {
		if(self->note==15||self->note ==17||self->note ==21||self->note ==23) SIO_WRITE(&sio0,1);
		else  SIO_WRITE(&sio0,0);
		AFTER(MSEC(500*interval),&controller,toggle_led,self->bpm);
	}
	self->note = (self->note+1)%32;
	if(self->note%boardNum == myRank){
		SEND(MSEC(tempo*500*interval-50),MSEC(50),&generator,gap,0);
    	SEND(MSEC(tempo*500*interval),MSEC(tempo*250*interval),self,startSound,self->bpm);
	}
    

}

int getBpm(Controller* self, int unused){
	return self->bpm;
}

void pause(Sound *self, int arg){
	self->play = ! self->play;
	if(self->play){
		SCI_WRITE(&sci0, "Playing \n");
		
		ASYNC(&generator, play,0);
		SIO_WRITE(&sio0,1);
	}else{
		SCI_WRITE(&sci0, "Paused \n");
		SIO_WRITE(&sio0,0);
	}
}
void pause_c(Controller *self, int arg){
	self->play = ! self->play;
	ASYNC(&controller,toggle_led, self->bpm);
	ASYNC(&controller,startSound,self->bpm);
}
 
void change_key(Controller *self, int num){
	if(num>=-5&&num<=5){
		self->key = num;
	}else{
		SCI_WRITE(&sci0, "Invalid key! \n");
	}
}
void change_bpm(Controller *self, int num){
	
	if(num>=30&&num<=300){
		self->bpm = num;
		

	}else{
		SCI_WRITE(&sci0, "Invalid BPM! \n");
	}
}	
/*protocol
 * msgId 1: inc the volume
 * msgId 2: dec the volume
 * msgId 3: mute
 * msgId 4: pause
 * msgId 5: change the positive key msg.buff = new value (buffer size = 1)
 * msgId 6: change the negative key msg.buff = new value (buffer size = 1)
 * msgId 7: change the bpm msg.buff = new value (buffer size = 3)
 * msgId 126: send the board number in the network. msg.buff = boardNum(buffer size = 2)
 * msgId 127: Traversing the board on the CAN network. msg.buff = current board number detected, used for myRank
 * 	msgid 125: reset the bpm to 120 and key to 0, nodeid is leader's rank
 * msgid 124: slave try to get leadership, old leader ack with newleader's rank
 */



void send_key_msg(App* self,int num){
   
   CANMsg msg;
   
   if (num < 0){
	   msg.msgId = 6;
   }
   else{
	   msg.msgId = 5;
   }
   msg.nodeId = self->myRank;
   char str_num[1]; 
   sprintf(str_num,"%d", abs(num));
   msg.length = 1;
   msg.buff[0] = str_num[0];
   //msg.buff[1] = 0;
   CAN_SEND(&can0, &msg);
}


void send_bpm_msg(App* self, int num){
    CANMsg msg;
    char str_num[3]; 
    sprintf(str_num,"%d", num);
    msg.msgId = 6;
    msg.nodeId = self->myRank;
    msg.length = 3;
    msg.buff[0] = str_num[0];
    msg.buff[1] = str_num[1];
	if(num>99)
		msg.buff[2] = str_num[2];
    CAN_SEND(&can0, &msg);
}

void send_note_msg(App* self, int noteId){
	CANMsg msg;
	msg.msgId = 9;
	msg.nodeId = self->myRank;
	char str_num[1]; 
	sprintf(str_num,"%c", noteId);// TODO: Use character send noteID
	msg.length = 1;
	msg.buff[0] = str_num[0];
	//msg.buff[1] = 0;
	CAN_SEND(&can0, &msg);
}

void set_print_flag(Controller* self, int num){
	self->print_flag =num;
}
void print_tempo(Controller *self, int num){
	if(self->print_flag==0) return;
	char strbuff[100];
	snprintf(strbuff,100,"Current tempo: %d\n",self->bpm);
	SCI_WRITE(&sci0,strbuff);
	AFTER(SEC(10),self,print_tempo,0);
}

void reader(App* self, int c)
{
	 SCI_WRITE(&sci0,"--------------------reader-------------------------\n");
     SCI_WRITE(&sci0, "Rcv: \'");
	 SCI_WRITECHAR(&sci0, c);
     SCI_WRITE(&sci0, "\'\n");
	 char strbuff[100];
	 int num;
	 CANMsg msg;

	switch(c) {
		case 'o':
			ASYNC(&committee,IorS_to_W,0);
			AFTER(MSEC(500),&committee,send_GetLeadership_msg,0);
			AFTER(SEC(1),&committee,change_StateAfterCompete,0);
			break;
		case 'k':
			
			self->c[self->count] = '\0';
			num = atoi(self->c);
	
			self->count = 0;
		// print_key(num);
			if(!self->mode){
				SYNC(&controller,change_key,num);
			}
			ASYNC(self,send_key_msg,num);
		break;
		case 'b':
		
			self->c[self->count] = '\0';
			num = atoi(self->c);
		
			self->count = 0;
			// print_key(num);
			if(!self->mode){
				SYNC(&controller,change_bpm,num);
			}
				
		
			ASYNC(self,send_bpm_msg,num);
		break;

		case 'q':
			//volume_control(&generator,1);
			if(!self->mode)
				ASYNC(&generator,volume_control,1);

			
			msg.msgId = 1;
			msg.nodeId = 0;
			msg.length = 0;
			CAN_SEND(&can0, &msg);

			break;
		
		case 'a':
			//SCI_WRITE(&sci0, "Down is pressed");
			//volume_control(&generator,0);
			if(!self->mode)
				ASYNC(&generator,volume_control,0);
		
			
			msg.msgId = 2;
			msg.nodeId = 0;
			msg.length = 0;
			CAN_SEND(&can0, &msg);	
			break;
		case 'm':
			//mute(&generator);
			if(self->mode==1){
				ASYNC(&generator,mute,0);

			}
			msg.msgId = 3;
			msg.nodeId = 0;
			msg.length = 0;
			CAN_SEND(&can0, &msg);
			break;
		case 'p':
			if(!self->mode){
				SYNC(&generator,pause,0);
				SYNC(&controller,pause_c,0);
			}
			
			msg.msgId = 4;
			msg.nodeId = 0;
			msg.length = 0;
			CAN_SEND(&can0, &msg);
			
			break;
		case 'x':
			snprintf(strbuff,100,"BoardNum: %d\nLeaderRank: %d\nMyRank: %d\n",self->boardNum,self->leaderRank,self->myRank);
			SCI_WRITE(&sci0,strbuff);
			break;
		case 'r':
			//New function: leader reset tempo and key for all boards
			if(self->mode ==0){
				//check if leader
				SCI_WRITE(&sci0,"Reseting BPM and key from master\n");
				ASYNC(self, send_Reset_msg, 0);
				SYNC(&controller, change_bpm, 120);
				SYNC(&controller, change_key, 0);
			}
			// if slave: ignore
			
			break;

		case 'e':
			//New function: enable or disable print tempo
			if(self->mode==0){
				if(self->print_flag==0){
					self->print_flag=1;
					SCI_WRITE(&sci0,"Print tempo function for master is enabled \n");
					ASYNC(&controller, set_print_flag,1);
					ASYNC(&controller, print_tempo, 0);
				}else{
					SCI_WRITE(&sci0,"Print tempo function for master is disabled \n");
					self->print_flag=0;
					ASYNC(&controller, set_print_flag,0);
				}			
			}
			if(self->mode ==1){
				if(self->print_flag==0){
					self->print_flag=1;
					SCI_WRITE(&sci0,"Print mute state function for slave is enabled \n");
					ASYNC(self,print_mute_state,0);
				}else{
					SCI_WRITE(&sci0,"Print mute state function for slave is disabled \n");
					self->print_flag=0;
				}	
			}

			break;
		case 't':
			//is slave: when 't' is pressed, toggle the state of mute
			if(self->mode==1){	
				ASYNC(&generator,mute,0);
				ASYNC(self,print_mute_state,0);
			}		
			break;
		case 'M':
			// Start or stop monitor
			if(self->monitorFlag == 1){
				self->monitorFlag = 0;
			}else{
				self->monitorFlag = 1;
			}
			break;
		//Function: Compulsory leadership change
		
			
	}
	if ((c >='0'&&c<='9') || (self->count==0 && c == '-')){
		self->c[self->count++] = c;

	}
	
}

void startApp(App* self, int arg)
{
    CANMsg msg;
	// Serial sci0 = initSerial(SCI_PORT0, &app, reader);
	// SysIO sio0 = initSysIO(SIO_PORT0, &app,user_call_back);
	// Can can0 = initCan(CAN_PORT0, &app, receiver);
    CAN_INIT(&can0);
    SCI_INIT(&sci0);
	SIO_INIT(&sio0);
	//configure it to call call-back when button is released
	//SIO_TRIG(&sio0,1);
	T_RESET(&self->timer);
	SIO_WRITE(&sio0,0);
    SCI_WRITE(&sci0, "Hello, hello...\n");
	initNetwork(self, 0);
//	SYNC(self, initNetwork, 0);
	SCI_WRITE(&sci0,"--------------------startApp-------------------------\n");
	 char strbuff[100];
	// snprintf(strbuff,100,"start App Mode: %d\nboardNum: %d\nmyRank: %d\nleaderRank: %d\n",self->mode, self->boardNum, self->myRank, self->leaderRank);
	// SCI_WRITE(&sci0,strbuff);
//	self->mode = -1;
    // msg.msgId = 1;
    // msg.nodeId = 1;
    // msg.length = 6;
    // msg.buff[0] = 'H';
    // msg.buff[1] = 'e';
    // msg.buff[2] = 'l';
    // msg.buff[3] = 'l';
    // msg.buff[4] = 'o';
    // msg.buff[5] = 0;
   // CAN_SEND(&can0, &msg);
	snprintf(strbuff,100,"Mode: %d\n",self->mode);
	SCI_WRITE(&sci0,strbuff);
	//detect members in the network
	ASYNC(&committee, initBoardNum, 0);
	SCI_WRITE(&sci0, "Ready for competing for leadership\n");
	AFTER(SEC(4), &committee, initMode, 0);
	// ASYNC(&controller,startSound,0);
	// ASYNC(&generator, play,0);
	AFTER(SEC(10), &app, monitor, 0);
}

int main()
{
    INSTALL(&sci0, sci_interrupt, SCI_IRQ0);
    INSTALL(&can0, can_interrupt, CAN_IRQ0);
    INSTALL(&sio0,sio_interrupt, SIO_IRQ0);
    TINYTIMBER(&app, startApp, 0);
    return 0;
}
