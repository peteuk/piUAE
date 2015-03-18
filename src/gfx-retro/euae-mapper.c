#include "libretro.h"

#include "libretro-euae.h"
#include "AMIGAkeymap.h"
#include "graph.h"
#include "vkbd.h"

#ifdef PS3PORT
#include "sys/sys_time.h"
#include "sys/timer.h"
#define usleep  sys_timer_usleep
#else
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#endif

unsigned short int bmp[1024*1024],savebmp[1024*1024];

int NPAGE=-1, KCOL=1, BKGCOLOR=0, MAXPAS=6;
int SHIFTON=-1,MOUSEMODE=-1,NUMJOY=0,SHOWKEY=-1,PAS=2,STATUTON=-1;

short signed int SNDBUF[1024*2];
int snd_sampler = 44100 / 60;
char RPATH[512];
char DSKNAME[512];

int gmx=320,gmy=240; //gui mouse

int al[2];//left analog1
int ar[2];//right analog1
unsigned char MXjoy0; // joy
int touch=-1; // gui mouse btn
int fmousex,fmousey; // emu mouse
int pauseg=0; //enter_gui
int SND=1; //SOUND ON/OFF
int NUMjoy=1;

static int mbt[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

long frame=0;
unsigned long  Ktime=0 , LastFPSTime=0;

int BOXDEC= 32+2;
int STAT_BASEY;

extern char Key_Sate[512];
extern char Key_Sate2[512];

extern bool opt_analog;

extern char * filebrowser(const char *path_and_name);

static retro_input_state_t input_state_cb;
static retro_input_poll_t input_poll_cb;

void retro_set_input_state(retro_input_state_t cb)
{
   	input_state_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb)
{
   	input_poll_cb = cb;
}

#ifdef AND
#define DEFAULT_PATH "/mnt/sdcard/euae/"
#else
	#ifdef PS3PORT
	#define DEFAULT_PATH "/dev_hdd0/HOMEBREW/UAE/"
	#else
	#define DEFAULT_PATH "/"
	#endif
#endif

#define MDEBUG
#ifdef MDEBUG
#define mprintf printf
#else
#define mprintf(...)
#endif


#include "sysconfig.h"
#include "sysdeps.h"

#include "options.h"
#include "gui.h"
#include "xwin.h"
#include "disk.h"


void enter_gui(){

	enter_options();

	return;
}

long GetTicks2(void)
{ // in MSec
#ifndef _ANDROID_

#ifdef PS3PORT

	//#warning "GetTick PS3\n"

	unsigned long	ticks_micro;
	uint64_t secs;
	uint64_t nsecs;

	sys_time_get_current_time(&secs, &nsecs);
	ticks_micro =  secs * 1000000UL + (nsecs / 1000);

	return ticks_micro/1000;
#else
   	struct timeval tv;
   	gettimeofday (&tv, NULL);
   	return (tv.tv_sec*1000000 + tv.tv_usec)/1000;
#endif

#else

    	struct timespec now;
    	clock_gettime(CLOCK_MONOTONIC, &now);
    	return (now.tv_sec*1000000 + now.tv_nsec/1000)/1000;
#endif

}
void gui_poll_events(){
	//NO SURE FIND BETTER WAY TO COME BACK IN MAIN THREAD IN HATARI GUI

	Ktime = GetTicks2();

	if(Ktime - LastFPSTime >= 1000/50){
		frame++;
 	    	LastFPSTime = Ktime;
		co_switch(mainThread);
		//retro_run();
 	}
}

void Print_Statut(){

	STAT_BASEY=CROP_HEIGHT;

	DrawFBoxBmp(bmp,0,CROP_HEIGHT,CROP_WIDTH,STAT_YSZ,RGB565(0,0,0));

	if(MOUSEMODE==-1)
		Draw_text(bmp,STAT_DECX,STAT_BASEY,0xffff,0x8080,1,2,40,"Joy  ");
	else
		Draw_text(bmp,STAT_DECX,STAT_BASEY,0xffff,0x8080,1,2,40,"Mouse");

	Draw_text(bmp,STAT_DECX+40 ,STAT_BASEY,0xffff,0x8080,1,2,40,(SHIFTON>0?"SHFT":""));
        Draw_text(bmp,STAT_DECX+80 ,STAT_BASEY,0xffff,0x8080,1,2,40,"MS:%d",PAS);
	Draw_text(bmp,STAT_DECX+120,STAT_BASEY,0xffff,0x8080,1,2,40,"Joy:%d",NUMjoy);

}

/*
L2  show/hide Statut
R2  toggle snd ON/OFF
L   show/hide vkbd
R   MOUSE SPEED(gui/emu)
SEL toggle mouse/joy mode
STR toggle num joy
A   fire/mousea/valid key in vkbd
B   mouseb
X   [unused fixme: switch Shift ON/OFF] / umount in gui
Y   Emu Gui
*/

void Screen_SetFullUpdate(){
	reset_drawing();
}

void Process_key(){

	int i;
	for(i=0;i<320;i++){

		Key_Sate[i]=input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0,i)?0x80:0;

		if(SDLKeyToAMIGAScanCode[i]==0x60/*AK_LSH*/ ){  //SHIFT CASE

			if( Key_Sate[i] && Key_Sate2[i]==0 ){

				if(SHIFTON == 1)
					retro_key_up(	SDLKeyToAMIGAScanCode[i] );
				else if(SHIFTON == -1)
					retro_key_down(SDLKeyToAMIGAScanCode[i] );

				SHIFTON=-SHIFTON;

				Key_Sate2[i]=1;

			}
			else if ( !Key_Sate[i] && Key_Sate2[i]==1 )Key_Sate2[i]=0;

		}
		else {

			if(Key_Sate[i] && SDLKeyToAMIGAScanCode[i]!=-1  && Key_Sate2[i]==0){
				retro_key_down(	SDLKeyToAMIGAScanCode[i] );
				Key_Sate2[i]=1;
			}
			else if ( !Key_Sate[i] && SDLKeyToAMIGAScanCode[i]!=-1 && Key_Sate2[i]==1 ) {
				retro_key_up( SDLKeyToAMIGAScanCode[i] );
				Key_Sate2[i]=0;

			}

		}
	}

}

void update_input(void)
{
	int i;
	//   RETRO      B    Y    SLT  STA  UP   DWN  LEFT RGT  A    X    L    R    L2   R2   L3   R3
        //   INDEX      0    1    2    3    4    5    6    7    8    9    10   11   12   13   14   15
	static int vbt[16]={0x1C,0x39,0x01,0x3B,0x01,0x02,0x04,0x08,0x80,0x6D,0x15,0x31,0x24,0x1F,0x6E,0x6F};
	static int oldi=-1;
	static int vkx=0,vky=0;

 	MXjoy0=0;
	if(oldi!=-1){retro_key_up(oldi);oldi=-1;}

   	input_poll_cb();
	Process_key();

        if (Key_Sate[RETROK_F11] || input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_Y) ){
		pauseg=1;
		//enter_gui(); //old
	}

	i=10;//show vkey toggle
	if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && mbt[i]==0 )
	    	mbt[i]=1;
	else if ( mbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) ){
	   	mbt[i]=0;
		SHOWKEY=-SHOWKEY;
		Screen_SetFullUpdate();
	}

	i=2;//mouse/joy toggle
	if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && mbt[i]==0 )
	    	mbt[i]=1;
	else if ( mbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) ){
	   	mbt[i]=0;
		MOUSEMODE=-MOUSEMODE;
	}

	i=3;//num joy toggle
	if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && mbt[i]==0 )
	    	mbt[i]=1;
	else if ( mbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) ){
	   	mbt[i]=0;
		NUMJOY++;if(NUMJOY>1)NUMJOY=0;
		NUMjoy=-NUMjoy;
	}

        i=11;//mouse gui speed
	if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && mbt[i]==0 )
	    	mbt[i]=1;
	else if ( mbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) ){
	   	mbt[i]=0;
		PAS++;if(PAS>MAXPAS)PAS=1;
	}


/*
//FIXME
        i=9;//switch shift On/Off
	if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && mbt[i]==0 )
	    	mbt[i]=1;
	else if ( mbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) ){
	   	mbt[i]=0;
		SHIFTON=-SHIFTON;
		Screen_SetFullUpdate();
	}
*/

	i=12;//show/hide statut
	if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && mbt[i]==0 )
	    	mbt[i]=1;
	else if ( mbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) ){
	   	mbt[i]=0;
		STATUTON=-STATUTON;
		Screen_SetFullUpdate();
	}

	i=13;//sonud on/off
	if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) && mbt[i]==0 )
	    	mbt[i]=1;
	else if ( mbt[i]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) ){
	   	mbt[i]=0;
		SND=-SND;
		//Screen_SetFullUpdate();
	}
	if(SHOWKEY==1){

		static int vkflag[5]={0,0,0,0,0};

		if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP) && vkflag[0]==0 )
		    	vkflag[0]=1;
		else if (vkflag[0]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP) ){
		   	vkflag[0]=0;
			vky -= 1;
		}

		if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN) && vkflag[1]==0 )
		    	vkflag[1]=1;
		else if (vkflag[1]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN) ){
		   	vkflag[1]=0;
			vky += 1;
		}

		if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT) && vkflag[2]==0 )
		    	vkflag[2]=1;
		else if (vkflag[2]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT) ){
		   	vkflag[2]=0;
			vkx -= 1;
		}

		if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) && vkflag[3]==0 )
		    	vkflag[3]=1;
		else if (vkflag[3]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) ){
		   	vkflag[3]=0;
			vkx += 1;
		}

		if(vkx<0)vkx=9;
		if(vkx>9)vkx=0;
		if(vky<0)vky=4;
		if(vky>4)vky=0;

		virtual_kdb(bmp,vkx,vky);

		i=8;
		if(input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i)  && vkflag[4]==0)
			vkflag[4]=1;
		else if( !input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i)  && vkflag[4]==1) {

			vkflag[4]=0;
			i=check_vkey2(vkx,vky);

			if(i==-2){
				NPAGE=-NPAGE;oldi=-1;
				//Clear interface zone
				Screen_SetFullUpdate();

			}
			else if(i==-1)oldi=-1;
			else if(i==-3){//KDB bgcolor
				Screen_SetFullUpdate();
				KCOL=-KCOL;
				oldi=-1;
			}
			else if(i==-4){//VKbd show/hide

				oldi=-1;
				Screen_SetFullUpdate();
				SHOWKEY=-SHOWKEY;
			}
			else if(i==-5){//Change Joy number
				NUMjoy=-NUMjoy;
				oldi=-1;
			}
			else {
				if(i==0x60/*AK_LSH*/){

					if(SHIFTON == 1)retro_key_up(i);
					else retro_key_down(i);

					SHIFTON=-SHIFTON;

					Screen_SetFullUpdate();

					oldi=-1;
				}
				else {
					oldi=i;
					retro_key_down(i);
				}
			}

		}

         	if(STATUTON==1)Print_Statut();

		return;
	}

   	static int mbL=0,mbR=0;
	int mouse_l;
	int mouse_r;
  	int16_t mouse_x;
   	int16_t mouse_y;

	if(MOUSEMODE==-1){ //Joy mode


                al[0] =(input_state_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_X));
                al[1] =(input_state_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_LEFT, RETRO_DEVICE_ID_ANALOG_Y));

		setjoybuttonstate (0,0,input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A));
		setjoybuttonstate (0,1,input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B));

if(opt_analog){
		setjoystickstate  (0, 0, al[0], 32767);
		setjoystickstate  (0, 1, al[1], 32767);
}
else{
		for(i=4;i<8/*9*/;i++)if( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, i) )MXjoy0 |= vbt[i]; // Joy press
		retro_joy0(MXjoy0);
}

	   	mouse_x = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
	   	mouse_y = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
	   	mouse_l    = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);
	   	mouse_r    = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT);

		fmousex=mouse_x;
  		fmousey=mouse_y;

	}
	else {  //Mouse mode
		fmousex=fmousey=0;

//ANALOG RIGHT
                ar[0] = (input_state_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_X));
                ar[1] = (input_state_cb(0, RETRO_DEVICE_ANALOG, RETRO_DEVICE_INDEX_ANALOG_RIGHT, RETRO_DEVICE_ID_ANALOG_Y));

		if(ar[0]<=-1024)fmousex-=(-ar[0])/1024;
		if(ar[0]>= 1024)fmousex+=( ar[0])/1024;
		if(ar[1]<=-1024)fmousey-=(-ar[1])/1024;
		if(ar[1]>= 1024)fmousey+=( ar[1])/1024;
//PAD
		if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT))fmousex += PAS;

		if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT))fmousex -= PAS;

		if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN))fmousey += PAS;

		if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP))fmousey -= PAS;

		mouse_l=input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A);
		mouse_r=input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B);
       }

	if(mbL==0 && mouse_l){
		mbL=1;
		retro_mouse_but0(1);
	}
	else if(mbL==1 && !mouse_l) {


		retro_mouse_but0(0);
                mbL=0;
	}

	if(mbR==0 && mouse_r){
		mbR=1;
		retro_mouse_but1(1);
	}
	else if(mbR==1 && !mouse_r) {

 		retro_mouse_but1(0);
                mbR=0;
	}

	retro_mouse(fmousex, fmousey);

	if(STATUTON==1)Print_Statut();

}

int input_gui()
{
	int SAVPAS=PAS;
	int ret=0;

   	input_poll_cb();

	int mouse_l;
	int mouse_r;
	int16_t mouse_x,mouse_y;
	mouse_x=mouse_y=0;

	//mouse/joy toggle
	if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, 2) && mbt[2]==0 )
	    	mbt[2]=1;
	else if ( mbt[2]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, 2) ){
	   	mbt[2]=0;
		MOUSEMODE=-MOUSEMODE;
	}

	if(MOUSEMODE==1){

//TODO FIX THIS :(
#if defined(PS3PORT)
 		//Slow Joypad Mouse Emulation for PS3
		static int pair=-1;
	 	pair=-pair;
		if(pair==1)return;
		PAS=1;
#elif defined(WIIPORT)
		PAS=1;
#endif

		if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT))mouse_x += PAS;
		if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT))mouse_x -= PAS;
		if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN))mouse_y += PAS;
		if (input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP))mouse_y -= PAS;
		mouse_l=input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A);
		mouse_r=input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B);

		PAS=SAVPAS;
	}
	else {

   		mouse_x = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_X);
   		mouse_y = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_Y);
   		mouse_l    = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_LEFT);
   		mouse_r    = input_state_cb(0, RETRO_DEVICE_MOUSE, 0, RETRO_DEVICE_ID_MOUSE_RIGHT);
	}

   	static int mmbL=0,mmbR=0;

	if(mmbL==0 && mouse_l){

		mmbL=1;
		touch=1;

	}
	else if(mmbL==1 && !mouse_l) {

                mmbL=0;
		touch=-1;
	}

	if(mmbR==0 && mouse_r){
		mmbR=1;
	}
	else if(mmbR==1 && !mouse_r) {
                mmbR=0;
	}

        gmx+=mouse_x;
        gmy+=mouse_y;
	if(gmx<0)gmx=0;
        if(gmx>retrow-1)gmx=retrow-1;
        if(gmy<0)gmy=0;
        if(gmy>retroh-1)gmy=retroh-1;
}

int update_input_gui(){

	int ret=0;
	static int dskflag[7]={0,0,0,0,0,0,0};// UP -1 DW 1 A 2 B 3 LFT -10 RGT 10 X 4

   	input_poll_cb();

	if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP) && dskflag[0]==0 ){
	    	dskflag[0]=1;
		ret= -1;
	}
	else /*if (dskflag[0]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP) ){
	   	dskflag[0]=0;
		ret= -1;
	}*/dskflag[0]=0;

	if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN) && dskflag[1]==0 ){
	    	dskflag[1]=1;
		ret= 1;
	}
	else/* if (dskflag[1]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN) ){
	   	dskflag[1]=0;
		ret= 1;
	}*/dskflag[1]=0;

	if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT) && dskflag[4]==0 )
	    	dskflag[4]=1;
	else if (dskflag[4]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT) ){
	   	dskflag[4]=0;
		ret= -10;
	}

	if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) && dskflag[5]==0 )
	    	dskflag[5]=1;
	else if (dskflag[5]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT) ){
	   	dskflag[5]=0;
		ret= 10;
	}

	if ( ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A) || \
		input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_LCTRL) ) && dskflag[2]==0 ){
	    	dskflag[2]=1;

	}
	else if (dskflag[2]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A)\
		&& !input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_LCTRL) ){

	   	dskflag[2]=0;
		ret= 2;
	}

	if ( ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B) || \
		input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_LALT) ) && dskflag[3]==0 ){
	    	dskflag[3]=1;
	}
	else if (dskflag[3]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B) &&\
		!input_state_cb(0, RETRO_DEVICE_KEYBOARD, 0, RETROK_LALT) ){

	   	dskflag[3]=0;
		ret= 3;
	}

	if ( input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X) && dskflag[6]==0 )
	    	dskflag[6]=1;
	else if (dskflag[6]==1 && ! input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X) ){
	   	dskflag[6]=0;
		ret= 4;
	}

	return ret;

}

