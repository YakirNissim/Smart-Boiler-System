//
// The project of Yakir and Kfir 
//
// HC05 Bluetooth module
// pin1 : KEY   N.C
// pin2 : VCC   to Vcc +5V
// pin3 : GND   to GND
// pin4 : TXD   to NUC140 UART0-RX (GPB0)
// pin5 : RXD   to NUC140 UART0-TX (GPB1)
// pin6 : STATE N.C.
// 
// Thermometer
// pin1 : S     to GPA6
// pin2 : GND   to GND
// pin3 : VCC   to VCC33
//
//Special symbols
//0x22,0x23 = clock symbol
//0x3b = thermometer symbol
//0x3e = candle symbol
//0x60 = bluetooth symbol


#define  ONESHOT  0   // counting and interrupt when reach TCMPR number, then stop
#define  PERIODIC 1   // counting and interrupt when reach TCMPR number, then counting from 0 again
#define  TOGGLE   2   // keep counting and interrupt when reach TCMPR number, tout toggled (between 0 and 1)
#define  CONTINUOUS 3 // keep counting and interrupt when reach TCMPR number

#include <stdio.h>
#include "Driver\DrvUART.h"
#include <stdbool.h>	
#include <math.h>
#include <string.h>
#include "NUC1xx.h"
#include "Driver\DrvSYS.h"
#include "Driver\DrvGPIO.h"
#include "LCD_Driver.h"
#include "scankey.h"

#define  INIT_LED1 DrvGPIO_Open(E_GPC, 12, E_IO_OUTPUT)
#define  INIT_BUZZER DrvGPIO_Open(E_GPB, 11, E_IO_OUTPUT) // initial GPIO pin GPB11 for controlling Buzzer
#define  LED1_ON   DrvGPIO_ClrBit(E_GPC, 12)
#define  LED1_OFF  DrvGPIO_SetBit(E_GPC, 12)
#define  Buzzer_ON  DrvGPIO_ClrBit(E_GPB,11) // GPB11 = 0 to turn on Buzzer
#define  Buzzer_OFF  DrvGPIO_SetBit(E_GPB,11) // GPB11 = 1 to turn off Buzzer
#define  I_Green DrvGPIO_Open(E_GPA, 13, E_IO_OUTPUT)
#define  I_Blue DrvGPIO_Open(E_GPA, 12, E_IO_OUTPUT)
#define  I_Red DrvGPIO_Open(E_GPA, 14, E_IO_OUTPUT)
#define  C_Green DrvGPIO_Close(E_GPA, 13)
#define  Blue_ON  DrvGPIO_ClrBit(E_GPA, 12) // GPA12 pin output Hi to turn on Blue  LED
#define  Red_ON  DrvGPIO_ClrBit(E_GPA, 14) // GPA14 pin output Hi to turn on Red  LED
#define  Green_ON  DrvGPIO_ClrBit(E_GPA, 13) // GPA13 pin output Hi to turn on Green  LED
#define  Blue_OFF  DrvGPIO_SetBit(E_GPA, 12) // GPA12 pin output Hi to turn off Blue  LED
#define  Red_OFF  DrvGPIO_SetBit(E_GPA, 14) // GPA14 pin output Hi to turn off Red  LED
#define  Green_OFF  DrvGPIO_SetBit(E_GPA, 13) // GPA13 pin output Hi to turn off Green  LED


#define g70 45875
#define max_temp 66
#define set_fun 1
#define shabat_fun 2
#define temp_fun 3
#define timer_fun 4 


uint8_t i = 0;
int8_t t = 0;
uint8_t s = 0;

bool cc = false;
bool cc2 = false;

int32_t msec = 0;
int32_t sec = 0;
int32_t min = 0;
int32_t hour = 0;

int32_t sec_t = 0;
int32_t min_t = 0;
int32_t hour_t = 0;

bool set_clock_on =false;

bool sel_timer=false;

bool boiler=false;

bool Set_clock=false;
bool set_timer=false;

int32_t min_timer = 0;
int32_t hour_timer = 0;

int32_t key =0;
uint8_t menu_mode = 0;

int32_t temp=0;

char cline[16]   =  "                ";
char clock_s[16] = "Clock:          ";
char xxx[50] = "--------------------------------------------------";
char state[16] = "State:          ";
char sstart[16] = "Start:          ";
char time[16]  = "Timer:          ";
char temp_s[16]   = "Temperature:    ";



uint8_t endline[4] = {0x0d,0x0a,0x0d,0x0a};
uint8_t value_temp=30;

uint32_t msec_read_buf = 0;
bool set_temp = false;
bool set_shabat = false;
bool sel_temp = false;
bool sel_shabat = false;
bool sel_boiler = false;
bool sel_on_off = false;

volatile uint8_t comRbuf[16];
volatile uint16_t comRbytes = 0;

char set_clock_r[9]  = "set clock";
char timer_r[6] = "timer:";
char shabat_mode_r[11] = "shabat mode";
char temp_r[4] = "temp";
char start_r[6] = "start:";
char on_r[2] = "on";
char off_r[3] = "off";
char exit_r[4] = "exit";
char menu_r[4] = "menu";

uint8_t hour_shabat_off = 0;
uint8_t min_shabat_off = 0;
int8_t hour_shabat_on = 0;
int8_t min_shabat_on = 0;


void check(void)
{
	if (comRbytes==2)//on
	{ 
		for (t=0;t<2;t++) {if(comRbuf[t] == on_r[t]) cc = true; else {cc = false; t=2;}}//on
		if (cc==true)//on 
		{
			if(menu_mode==1 || menu_mode==2 || menu_mode==3) sel_on_off=true;
			if(menu_mode==0 && sel_temp==false && sel_timer==false){sel_boiler=true;boiler=true;}
			cc=false;comRbytes=0;
		}
	}
	if (comRbytes==3)//off
	{			
		for (t=0;t<3;t++) {if(comRbuf[t] == off_r[t]) cc = true; else {cc = false; t=3;}}//off
		if (cc==true)//off 
		{
			if(menu_mode==1 || menu_mode==2 || menu_mode==3) sel_on_off=false;
			if(menu_mode==0){sel_temp=false; sel_boiler=false; boiler=false; min_t=0; hour_t=0; sec_t=0; sel_timer=false; sel_temp==false;}
			cc=false;comRbytes=0;
		}
	}
	if (comRbytes==4)//menu, exit, temp, xx.c 
	{
		if (menu_mode==2)//xx.c
		{
			for (t=0;t<2;t++) {if (comRbuf[t] >= 0x30 && comRbuf[t] <= 0x39) cc=true; else {cc=false; t=2;}}
			if (comRbuf[2]!='.' || comRbuf[3]!='c') cc=false;
			if (cc==true)
			{					
				if (((comRbuf[0] - 0x30)*10 + (comRbuf[1] - 0x30))>=10 && ((comRbuf[0] - 0x30)*10 + (comRbuf[1] - 0x30))<=65)
				value_temp = ((comRbuf[0] - 0x30)*10 + (comRbuf[1] - 0x30));
				else
				{
					DrvUART_Write(UART_PORT0,endline,2);//===================================================================
					DrvUART_Write(UART_PORT0,xxx,50);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"Error:",6);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"The temperature set is too low or too high",42);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"The temperature range of the boiler is->  10~65 .C",50);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"Please try again",16);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,xxx,50);
					DrvUART_Write(UART_PORT0,endline,4);
				}
				comRbytes=0; cc=false;
			}
		}
		for (t=0;t<4;t++) {if(comRbuf[t] == menu_r[t]) cc = true; else {cc = false; t=4;}}//menu
		if(cc==true)//menu 
		{
			switch (menu_mode)
			{
				case 0://main menu=======================================================================================
				{
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,xxx,50);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"Please send one of the options:",31);
					DrvUART_Write(UART_PORT0,endline,4);
					DrvUART_Write(UART_PORT0,"* Set clock",11);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"* on/off",8);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"* Timer",7);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"* Shabat mode",13);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"* Temp (for Temp Control)",25);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,xxx,50);
					DrvUART_Write(UART_PORT0,endline,4);
				}break;				
				case 1://timer menu=====================================================================================
				{
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,xxx,50);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"Please send one of the options:",31);
					DrvUART_Write(UART_PORT0,endline,4);
					DrvUART_Write(UART_PORT0,"* on/off",8);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"* hh:mm (for timer direction)",29);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"* exit",6);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,xxx,50);
					DrvUART_Write(UART_PORT0,endline,4);
				}break;
				case 2://temperature menu=============================================================================
				{
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,xxx,50);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"Please send one of the options:",31);
					DrvUART_Write(UART_PORT0,endline,4);
					DrvUART_Write(UART_PORT0,"* on/off",8);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"* xx.c (for Target temperature)",31);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"* exit",6);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,xxx,50);
					DrvUART_Write(UART_PORT0,endline,4);
				}break;
				case 3://Shabat menu===========================================================================================
				{
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,xxx,50);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"Please send one of the options:",31);
					DrvUART_Write(UART_PORT0,endline,4);
					DrvUART_Write(UART_PORT0,"* on/off",8);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"* start:hh:mm (for Start time",29);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"                                       direction)",49);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"* timer:hh:mm (for how long",27);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"                                              work)",51);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"* exit",6);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,xxx,50);
					DrvUART_Write(UART_PORT0,endline,4);
				}break;
				case 4://Set_Clock menu=====================================================================
				{
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,xxx,50);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"Please send one of the options:",31);
					DrvUART_Write(UART_PORT0,endline,4);
					DrvUART_Write(UART_PORT0,"* hh:mm (for clock direction)",29);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"* exit",6);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,xxx,50);
					DrvUART_Write(UART_PORT0,endline,4);
				}break;
			}					
			
			comRbytes=0;
			cc=false;
		}

		for (t=0;t<4;t++) if(comRbuf[t] == exit_r[t]) cc = true; else {cc = false; t=4;}//exit
		if (cc==true) //exit
		{
			sprintf(cline,"                ");
			Set_clock=false;
			set_timer=false;
			set_temp=false;
			set_shabat=false;
			menu_mode=0;
			DrvUART_Write(UART_PORT0,endline,2);//main menu=====================================================================
			DrvUART_Write(UART_PORT0,xxx,50);
			DrvUART_Write(UART_PORT0,endline,2);
			DrvUART_Write(UART_PORT0,"Please send one of the options:",31);
			DrvUART_Write(UART_PORT0,endline,4);
			DrvUART_Write(UART_PORT0,"* Set clock",11);
			DrvUART_Write(UART_PORT0,endline,2);
			DrvUART_Write(UART_PORT0,"* on/off",8);
			DrvUART_Write(UART_PORT0,endline,2);
			DrvUART_Write(UART_PORT0,"* Timer",7);
			DrvUART_Write(UART_PORT0,endline,2);
			DrvUART_Write(UART_PORT0,"* Shabat mode",13);
			DrvUART_Write(UART_PORT0,endline,2);
			DrvUART_Write(UART_PORT0,"* Temp (for Temp Control)",25);
			DrvUART_Write(UART_PORT0,endline,2);
			DrvUART_Write(UART_PORT0,xxx,50);
			DrvUART_Write(UART_PORT0,endline,4);
			cc=false;comRbytes=0;
		}
		if(menu_mode==0)//temp
		{
			for (t=0;t<4;t++) {if(comRbuf[t] == temp_r[t]) cc = true; else {cc = false; t=4;}}
			if(cc==true) {key=temp_fun; comRbytes=0;cc=false;}
		}
	}
	if (comRbytes==5)//timer, hh:mm, xxx.c
	{
		if (menu_mode==2)//xxx.c
		{
			for (t=0;t<3;t++) {if (comRbuf[t] >= 0x30 && comRbuf[t] <= 0x39) cc=true; else {cc=false; t=3;}}
			if (comRbuf[3]!='.' || comRbuf[4]!='c') cc=false;
			if (cc==true)
			{					
				DrvUART_Write(UART_PORT0,endline,2);//===================================================================
				DrvUART_Write(UART_PORT0,xxx,50);
				DrvUART_Write(UART_PORT0,endline,2);
				DrvUART_Write(UART_PORT0,"Error:",6);
				DrvUART_Write(UART_PORT0,endline,2);
				DrvUART_Write(UART_PORT0,"The temperature set is too low or too high",42);
				DrvUART_Write(UART_PORT0,endline,2);
				DrvUART_Write(UART_PORT0,"The temperature range of the boiler is->  10~65.C",49);
				DrvUART_Write(UART_PORT0,endline,2);
				DrvUART_Write(UART_PORT0,"Please try again",16);
				DrvUART_Write(UART_PORT0,endline,2);
				DrvUART_Write(UART_PORT0,xxx,50);
				DrvUART_Write(UART_PORT0,endline,4);
				comRbytes=0; cc=false;
			}
		}
		if (menu_mode==0)//timer
		{
			for (t=0;t<5;t++) {if(comRbuf[t] == timer_r[t]) cc = true; else {cc = false; t=5;}}
			if(cc==true) {key=timer_fun; cc=false;comRbytes=0;}//timer
		}
		if (menu_mode==1 || menu_mode==4)//hh:mm
		{
			for (t=0;t<5;t++)
			{
				if (t!=2 && comRbuf[t] >= 0x30 && comRbuf[t] <= 0x39) cc=true;
				else if (t!=2){cc=false; t=5;}
				if (comRbuf[2] != ':') cc=false;
			}
			if (cc==true)
			{
				if (((comRbuf[0] - 0x30)*10 + (comRbuf[1] - 0x30))<=23 && ((comRbuf[3] - 0x30)*10 + (comRbuf[4] - 0x30))<=59)
				{
					if(Set_clock==true)
					{
						hour = ((comRbuf[0] - 0x30)*10 + (comRbuf[1] - 0x30));
						min =  ((comRbuf[3] - 0x30)*10 + (comRbuf[4] - 0x30));
						sec = 0;
					}
					else
					{
						hour_timer = ((comRbuf[0] - 0x30)*10 + (comRbuf[1] - 0x30));
						min_timer =  ((comRbuf[3] - 0x30)*10 + (comRbuf[4] - 0x30));
					}
					
					
				}
				else
				{
					DrvUART_Write(UART_PORT0,endline,2);//===================================================================
					DrvUART_Write(UART_PORT0,xxx,50);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"Error:",6);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"The time you set a clock is unacceptable->  00~23:00~59",55);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"Please try again",16);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,xxx,50);
					DrvUART_Write(UART_PORT0,endline,4);
				}
				cc=false;comRbytes=0; 
			}
		}
	}
	if (comRbytes==9)//set clock
	{
		if(menu_mode==0)
		{
			for (t=0;t<9;t++) {if(comRbuf[t] == set_clock_r[t]) cc = true; else {cc = false; t=9;}}//set clock
			if(cc==true) {key=set_fun; comRbytes=0;cc=false;}//set clock
		}
	}
	if (comRbytes==11)//start:hh:mm, timer:hh:mm, shabat mode
	{
		if (menu_mode==3)//start:hh:mm, timer:hh:mm
		{
			for (t=0;t<6;t++){if(comRbuf[t] == start_r[t]) cc = true; else {cc = false; t=6;}}//start:hh:mm
			for (t=0;t<6;t++){if(comRbuf[t] == timer_r[t]) cc2 = true; else {cc2 = false; t=6;}}//timer:hh:mm
			if(cc==true)//start:hh:mm
			{
				if (((comRbuf[6] - 0x30)*10 + (comRbuf[7] - 0x30))<=23 && comRbuf[8]==':' && ((comRbuf[9] - 0x30)*10 + (comRbuf[10] - 0x30))<=59)
				{
					hour_shabat_on = ((comRbuf[6] - 0x30)*10 + (comRbuf[7] - 0x30));
					min_shabat_on =  ((comRbuf[9] - 0x30)*10 + (comRbuf[10] - 0x30));
				}
				else
				{
					DrvUART_Write(UART_PORT0,endline,2);//=========================================================================
					DrvUART_Write(UART_PORT0,xxx,50);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"Error:",6);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"What is obtained does not meet the start structure->  start:00~23:00~59",71);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"Please try again",16);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,xxx,50);
					DrvUART_Write(UART_PORT0,endline,4);
				}
				comRbytes=0;cc=false;
			}
			if(cc2==true)//timer:hh:mm
			{
				if (((comRbuf[6] - 0x30)*10 + (comRbuf[7] - 0x30))<=23 && comRbuf[8]==':' && ((comRbuf[9] - 0x30)*10 + (comRbuf[10] - 0x30))<=59)
				{
					hour_shabat_off = ((comRbuf[6] - 0x30)*10 + (comRbuf[7] - 0x30));
					min_shabat_off =  ((comRbuf[9] - 0x30)*10 + (comRbuf[10] - 0x30));
				}
				else
				{
					DrvUART_Write(UART_PORT0,endline,2);//=============================================================================
					DrvUART_Write(UART_PORT0,xxx,50);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"Error:",6);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"What is obtained does not meet the timer structure->  Timer:00~23:00~59",71);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"Please try again",16);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,xxx,50);
					DrvUART_Write(UART_PORT0,endline,4);
				}
				comRbytes=0;cc2=false;
			}
		}
		if(menu_mode==0)//shabat mode
		{
			for (t=0;t<11;t++) {if(comRbuf[t] == shabat_mode_r[t]) cc = true; else {cc = false; t=11;}}
			if(cc==true) {key=shabat_fun; comRbytes=0;cc=false;}
		}
	}
	if (comRbytes==12)//start:_hh:mm, timer:_hh:mm
	{
		if (menu_mode==3)//start:_hh:mm, timer_:hh:mm
		{
			for (t=0;t<6;t++){if(comRbuf[t] == start_r[t]) cc = true; else {cc = false; t=6;}}//start:hh:mm
			for (t=0;t<6;t++){if(comRbuf[t] == timer_r[t]) cc2 = true; else {cc2 = false; t=6;}}//timer:hh:mm
			if(cc==true)//start:_hh:mm
			{
				if (comRbuf[6]==' ')
				{
					DrvUART_Write(UART_PORT0,endline,2);//=========================================================================
					DrvUART_Write(UART_PORT0,xxx,50);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"Error:",6);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"There is extra space!",21);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"not good: start: hh:mm",22);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"    good: start:hh:mm",21);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"Please try again",16);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,xxx,50);
					DrvUART_Write(UART_PORT0,endline,4);
				}
				comRbytes=0;cc=false;
			}
			if(cc2==true)//timer:_hh:mm
			{
				if (comRbuf[6]==' ') 
				{
					DrvUART_Write(UART_PORT0,endline,2);//=======================================================================
					DrvUART_Write(UART_PORT0,xxx,50);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"Error:",6);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"There is extra space!",21);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"not good: timer: hh:mm",22);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"    good: timer:hh:mm",21);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,"Please try again",16);
					DrvUART_Write(UART_PORT0,endline,2);
					DrvUART_Write(UART_PORT0,xxx,50);
					DrvUART_Write(UART_PORT0,endline,4);
				}
				comRbytes=0;cc2=false;
			}
		}
	}
}
void InitPIN(void)
{
	INIT_BUZZER;
	INIT_LED1;
	LED1_OFF;
	I_Green;
	I_Blue;
	I_Red;
	Green_OFF;
	Blue_OFF;
	Red_OFF;
}
void InitPWM(void)
{	
	 SYSCLK->CLKSEL1.PWM01_S = 3; // Select 12Mhz for PWM clock source		
	 SYSCLK->APBCLK.PWM01_EN =1;  // Enable PWM0 & PWM1 clock	
	 PWMA->PPR.CP01=1;			      // Prescaler 0~255, Setting 0 to stop output clock
	 PWMA->CSR.CSR1=0;			      // PWM clock = clock source/(Prescaler + 1)/divider
	 PWMA->PCR.CH1MOD=1;			    // 0:One-shot mode, 1:Auto-load mode
																// CNR and CMR will be auto-cleared after setting CH1MOD form 0 to 1.	
	 PWMA->CNR1=0xFFFF;           // CNR : counting down   // PWM output high if CMRx+1 >= CNR
	 PWMA->CMR1=0xFFFF;		        // CMR : fix to compare  // PWM output low  if CMRx+1 <  CNR		
	 PWMA->PCR.CH1INV=0;          // Inverter->0:off, 1:on
	 PWMA->PCR.CH1EN=1;			      // PWM function->0:Disable, 1:Enable
	 PWMA->POE.PWM1=1;
}
void Blue_fun(void)
{
	SYS->GPAMFP.PWM1_AD14=0;
	I_Green;
	Green_OFF;
	Red_OFF;
	Blue_ON;
}
void Red_fun(void)
{
	SYS->GPAMFP.PWM1_AD14=0;
	I_Green;
	Green_OFF;
	Blue_OFF;
	Red_ON;
}
void Orange_fun(void)
{
	C_Green;
	SYS->GPAMFP.PWM1_AD14=1;
	PWMA->CMR1=g70;
	Blue_OFF;
	Red_ON;
}
void UART_INT_HANDLE(void) // UART interrupt subroutine 
{
	uint8_t bInChar[1] = {0xFF};

	while(UART0->ISR.RDA_IF==1) 
	{
		DrvUART_Read(UART_PORT0,bInChar,1);
		if(comRbytes < 16) // check if Buffer is full
		{
			if(bInChar[0]<=0x5a && bInChar[0]>=0x41) bInChar[0]=bInChar[0]+0x20;
			comRbuf[comRbytes] = bInChar[0];				
			comRbytes++;
		}	
		if (comRbytes>15) comRbytes=0; 
	}
}
void Thermistor(void)  // Thermistor subroutine 
{
	double Temp;
	uint32_t adc_value;
	adc_value = ADC->ADDR[6].RSLT; 
	Temp = log(10000.0 * (4096.0 / adc_value - 1));
	Temp = 1 /(0.001129148 + 0.000234125 * Temp + 0.0000000876741 * Temp * Temp * Temp);
	Temp = Temp - 273.15; // convert from Kelvin to Celsius
	temp = (uint32_t)Temp;
	ADC->ADCR.ADST=1;
	if(temp<=36)Blue_fun();
	if(temp>36 && temp<45)Orange_fun();
	if(temp>=45)Red_fun();
}
void InitTIMER0(void)// Timer0 initialize to tick every 1ms
{
	/* Step 1. Enable and Select Timer clock source */          
	SYSCLK->CLKSEL1.TMR0_S = 0;	//Select 12Mhz for Timer0 clock source 
	SYSCLK->APBCLK.TMR0_EN =1;	//Enable Timer0 clock source

	/* Step 2. Select Operation mode */	
	TIMER0->TCSR.MODE = PERIODIC;		//Select once mode for operation mode

	/* Step 3. Select Time out period = (Period of timer clock input) * (8-bit Prescale + 1) * (24-bit TCMP)*/
	TIMER0->TCSR.PRESCALE = 11;	// Set Prescale [0~255]
	TIMER0->TCMPR = 1000;		// Set TCMPR [0~16777215]
	//Timeout period = (1 / 12MHz) * ( 11 + 1 ) * 1,000 = 1 ms

	/* Step 4. Enable interrupt */
	TIMER0->TCSR.IE = 1;
	TIMER0->TISR.TIF = 1;		//Write 1 to clear for safty		
	NVIC_EnableIRQ(TMR0_IRQn);	//Enable Timer0 Interrupt

	/* Step 5. Enable Timer module */
	TIMER0->TCSR.CRST = 1;	//Reset up counter
	TIMER0->TCSR.CEN = 1;		//Enable Timer0
}

void TMR0_IRQHandler(void) // Timer0 interrupt subroutine 
{
	static uint32_t msec=0;

	msec++;
	if (comRbytes!=0) msec_read_buf++;
	else msec_read_buf=0;
	if (msec_read_buf==16 && comRbytes!=0){check(); comRbytes=0; msec_read_buf=0;}
	if(msec==1000)	//1 Second period
	{
		if(sel_timer==true || sel_temp==true || sel_shabat==true || menu_mode!=0) s++;
		if(s>2 || (sel_timer==false && sel_temp==false && sel_shabat==false && menu_mode==0)) s=0;
		msec = 0;
		sec++;
		if (sec==60)
		{
			sec = 0;
			min++;
			if (min==60)
			{
				min= 0;
				hour++;
				if (hour==24)hour = 0;
			}
		}
		Thermistor();//Temperature sampling
		if (sel_timer==false && sel_shabat==false){sec_t=0; min_t=0; hour_t=0;}
		Buzzer_OFF;
		if (sec_t==0 && min_t==0 && hour_t==0 && sel_temp==false && sel_boiler==false) boiler=false;
		if(temp>=max_temp || ( sel_temp==true && value_temp<=temp) || (sec_t==1 && min_t==0 && hour_t==0)) {Buzzer_ON;  sel_temp=false;}
		if (sel_temp==true && value_temp>temp) boiler=true;
		if (sec_t>0 || min_t>0 || hour_t>0)
		{
			sel_boiler=false;
			boiler=true;
			if (sec_t==1 && min_t==0 && hour_t==0 && sel_temp==false){sel_timer=false; sel_shabat=false;}
			sec_t--;
			if (sec_t<0&&(min_t>0||hour_t>0))
			{
				sec_t=59;
				min_t--;
				if (min_t<0&&hour_t>0)
				{
					min_t=59;
					hour_t--;
				}
			}
		}
		if (boiler==true)LED1_ON;
		if (boiler==false)LED1_OFF;
		if (hour==hour_shabat_on && min==min_shabat_on && sec==0 && sel_shabat==true)//timer shabat start
		{
			sel_boiler=false;
			sel_temp=false;
			sel_timer=true;
			sel_shabat=false;
			sec_t=0;
			hour_t=hour_shabat_off;
			min_t=min_shabat_off;
		}
	}
	TIMER0->TISR.TIF =1;
}

void InitADC(void)
{
	/* Step 1. GPIO initial */ 
	GPIOA->OFFD|=0x00400000; 	//Disable digital input path
	SYS->GPAMFP.ADC6_AD7=1; 	//Set ADC function 
				
	/* Step 2. Enable and Select ADC clock source, and then enable ADC module */          
	SYSCLK->CLKSEL1.ADC_S = 2;	//Select 12Mhz for ADC
	SYSCLK->CLKDIV.ADC_N = 1;	//ADC clock source = 12Mhz/2 = 6Mhz;
	SYSCLK->APBCLK.ADC_EN = 1;	//Enable clock source
	ADC->ADCR.ADEN = 1;			//Enable ADC module

	/* Step 3. Select Operation mode */
	ADC->ADCR.DIFFEN = 0;     	//single end input
	ADC->ADCR.ADMD   = 0;     	//single mode
		
	/* Step 4. Select ADC channel */
	ADC->ADCHER.CHEN = 0x40;    // channel bit [7:0]
	
	/* Step 5. Enable WDT module */
	ADC->ADCR.ADST=1;
}
void timer (void) //menu_mode=1
{
	clr_all_panel();
	sel_on_off=sel_timer;
	menu_mode=1;
	DrvUART_Write(UART_PORT0,endline,2);//timer menu==========================================
	DrvUART_Write(UART_PORT0,xxx,50);
	DrvUART_Write(UART_PORT0,endline,2);
	DrvUART_Write(UART_PORT0,"Please send one of the options:",31);
	DrvUART_Write(UART_PORT0,endline,4);
	DrvUART_Write(UART_PORT0,"* on/off",8);
	DrvUART_Write(UART_PORT0,endline,2);
	DrvUART_Write(UART_PORT0,"* hh:mm (for timer direction)",29);
	DrvUART_Write(UART_PORT0,endline,2);
	DrvUART_Write(UART_PORT0,"* exit",6);
	DrvUART_Write(UART_PORT0,endline,2);
	DrvUART_Write(UART_PORT0,xxx,50);
	DrvUART_Write(UART_PORT0,endline,4);
	while(set_timer == true)
	{
		sprintf(cline,"                ");
		sprintf(cline+6,"        "); sprintf(cline+14,"%c",(0x22+2*s)); sprintf(cline+15,"%c",(0x23+2*s));// clock symbol
		print_lcd(0, cline);
		sprintf(state,"State:          ");
		if (sel_on_off==false)sprintf(state+7,"off      ");
		if (sel_on_off==true) sprintf(state+7,"on       ");
		print_lcd(1, state);
		sprintf(time,"Timer:          ");
		if (hour_timer<10) sprintf(time+7,"0%d:", hour_timer);
		else               sprintf(time+7,"%2d:", hour_timer);   
		if (min_timer<10)  sprintf(time+10,"0%d", min_timer);
		else               sprintf(time+10,"%2d", min_timer);
		sprintf(time+13,"   ");
		print_lcd(2, time);
	}
  sel_timer=sel_on_off;
	if(sel_timer==true){sec_t=0; min_t=min_timer; hour_t=hour_timer;sel_temp=false; sel_shabat=false; sel_boiler=false;}
	clr_all_panel();
}

void temperature (void) //menu_mode=2
{
	clr_all_panel();
	sel_on_off=sel_temp;
	menu_mode=2;
	DrvUART_Write(UART_PORT0,endline,2);//temperature menu================================================
	DrvUART_Write(UART_PORT0,xxx,50);
	DrvUART_Write(UART_PORT0,endline,2);
	DrvUART_Write(UART_PORT0,"Please send one of the options:",31);
	DrvUART_Write(UART_PORT0,endline,4);
	DrvUART_Write(UART_PORT0,"* on/off",8);
	DrvUART_Write(UART_PORT0,endline,2);
	DrvUART_Write(UART_PORT0,"* xx.c (for target temperature)",31);
	DrvUART_Write(UART_PORT0,endline,2);
	DrvUART_Write(UART_PORT0,"* exit",6);
	DrvUART_Write(UART_PORT0,endline,2);
	DrvUART_Write(UART_PORT0,xxx,50);
	DrvUART_Write(UART_PORT0,endline,4);
	while(set_temp == true)
	{
		sprintf(cline,"                ");
		sprintf(cline+6,"         "); sprintf(cline+15,"%c",(0x3b+s));// thermometer symbol
		print_lcd(0, cline);
		sprintf(state,"State:          ");
		if (sel_on_off==false) sprintf(state+7,"off      ");
		if (sel_on_off==true)  sprintf(state+7,"on       ");
		print_lcd(1, state);
		sprintf(cline,"                ");
		sprintf(cline,"Temperature:     ");
		sprintf(cline,"Target temp:%d.c",value_temp);
		print_lcd(2, cline);
	}
	sel_temp=sel_on_off;
	if(sel_temp==true){sel_boiler=false; sel_shabat=false; sel_timer=false; sec_t = 0; min_t = 0; hour_t = 0;}
  clr_all_panel();
}
void Shabat (void) //menu_mode=3
{
	clr_all_panel();
	sel_on_off=sel_shabat;
	menu_mode=3;
	DrvUART_Write(UART_PORT0,endline,2);//Shabat menu===============================================================
	DrvUART_Write(UART_PORT0,xxx,50);
	DrvUART_Write(UART_PORT0,endline,2);
	DrvUART_Write(UART_PORT0,"Please send one of the options:",31);
	DrvUART_Write(UART_PORT0,endline,4);
	DrvUART_Write(UART_PORT0,"* on/off",8);
	DrvUART_Write(UART_PORT0,endline,2);
	DrvUART_Write(UART_PORT0,"* start:hh:mm (for Start time",29);
	DrvUART_Write(UART_PORT0,endline,2);
	DrvUART_Write(UART_PORT0,"                                       direction)",49);
	DrvUART_Write(UART_PORT0,endline,2);
	DrvUART_Write(UART_PORT0,"* timer:hh:mm (for how long",27);
	DrvUART_Write(UART_PORT0,endline,2);
	DrvUART_Write(UART_PORT0,"                                              work)",51);
	DrvUART_Write(UART_PORT0,endline,2);
	DrvUART_Write(UART_PORT0,"* exit",6);
	DrvUART_Write(UART_PORT0,endline,2);
	DrvUART_Write(UART_PORT0,xxx,50);
	DrvUART_Write(UART_PORT0,endline,4);
	while(set_shabat == true)
	{
		sprintf(cline,"                ");
		sprintf(cline+6,"        "); sprintf(cline+14,"%c",(0x3e+s)); sprintf(cline+15,"%c",(0x3e+s));// candle symbol
		print_lcd(0, cline);
		sprintf(state,"State:          ");
		if (sel_on_off==false)sprintf(state+7,"off      ");
		if (sel_on_off==true) sprintf(state+7,"on       ");
		print_lcd(1, state);
		sprintf(sstart,"Start:          ");
		if (hour_shabat_on<10) sprintf(sstart+7,"0%d:", hour_shabat_on);
		else               sprintf(sstart+7,"%2d:", hour_shabat_on);   
		if (min_shabat_on<10)  sprintf(sstart+10,"0%d", min_shabat_on);
		else               sprintf(sstart+10,"%2d", min_shabat_on);
		sprintf(sstart+13,"   ");
		print_lcd(2, sstart);
		sprintf(time, "Timer:          ");
		if (hour_shabat_off<10) sprintf(time+7,"0%d:", hour_shabat_off);
		else               sprintf(time+7,"%2d:", hour_shabat_off);   
		if (min_shabat_off<10)  sprintf(time+10,"0%d", min_shabat_off);
		else               sprintf(time+10,"%2d", min_shabat_off);
		sprintf(time+13,"   ");
		print_lcd(3, time);
	}
	sel_shabat=sel_on_off;
	if(sel_shabat==true){min_t=0; hour_t=0; sec_t=0; sel_timer=false;}
  clr_all_panel();
}
void Set_Clock (void) //menu_mode=4
{
	i=0;
	set_clock_on=true;
	clr_all_panel();
	menu_mode=4;
	DrvUART_Write(UART_PORT0,endline,2);//Set_Clock menu================================================================
	DrvUART_Write(UART_PORT0,xxx,50);
	DrvUART_Write(UART_PORT0,endline,2);
	DrvUART_Write(UART_PORT0,"Please send one of the options:",31);
	DrvUART_Write(UART_PORT0,endline,4);
	DrvUART_Write(UART_PORT0,"* hh:mm (for clock direction)",29);
	DrvUART_Write(UART_PORT0,endline,2);
	DrvUART_Write(UART_PORT0,"* exit",6);
	DrvUART_Write(UART_PORT0,endline,2);
	DrvUART_Write(UART_PORT0,xxx,50);
	DrvUART_Write(UART_PORT0,endline,4);
	while (Set_clock==true)
	{
		i++;
		if (i>20){i=0; set_clock_on=!set_clock_on;}
		print_lcd(1, "Set Clock:      ");
		if (set_clock_on==true)
		{
			sprintf(cline,"                ");
			if (hour<10)    sprintf(cline+8,"0%d:", hour);
			else            sprintf(cline+8,"%2d:", hour);   
			if (min<10)  sprintf(cline+11,"0%d:", min);
			else            sprintf(cline+11,"%2d:", min);
			if (sec<10)  sprintf(cline+14,"0%d", sec);
			else            sprintf(cline+14,"%2d", sec);
			print_lcd(2, cline);
		}
		else
		{
			sprintf(cline,"                ");
			sprintf(cline+13,":");
			if (sec<10)  sprintf(cline+14,"0%d", sec);
			else            sprintf(cline+14,"%2d", sec);
			print_lcd(2, cline);
		}
	}
	clr_all_panel();
}
int32_t main (void) //menu_mode=0
{
		STR_UART_T sParam;
    UNLOCKREG();
    DrvSYS_Open(48000000);
    LOCKREG();
   	
    DrvGPIO_InitFunction(E_FUNC_UART0); // Set UART pins

    //* UART Setting */
    sParam.u32BaudRate 	= 9600;
		sParam.u8cDataBits 	= DRVUART_DATABITS_8;
    sParam.u8cStopBits 	= DRVUART_STOPBITS_1;
    sParam.u8cParity 	= DRVUART_PARITY_NONE;
    sParam.u8cRxTriggerLevel= DRVUART_FIFO_1BYTES;

		// 	/* Set UART Configuration */
		if(DrvUART_Open(UART_PORT0,&sParam) != E_SUCCESS);
	
		UNLOCKREG();
		SYSCLK->PWRCON.XTL12M_EN = 1;
		SYSCLK->CLKSEL0.HCLK_S = 0;
		DrvSYS_Delay(5000); // wait till 12MHz crystal stable
		LOCKREG();	
		
		Initial_panel();  //initialize LCD
		clr_all_panel();  //clear LCD
		InitTIMER0();
		InitADC();
		InitPWM();
		InitPIN();
		DrvUART_EnableInt(UART_PORT0, DRVUART_RDAINT, UART_INT_HANDLE);
				
		sec=0;
		min=0;
		hour=0;
	
	video();
	for(t=0;t<3;t++)
	{
		for(i=0;i<3;i++)
		{
			print_lcd(0, "  Applications  ");
			sprintf(cline,"                ");
			sprintf(cline,"  ");
			sprintf(cline+2,"%c",0x60);// bluetooth symbol
			sprintf(cline+3,"  ");
			sprintf(cline+5,"%c",(0x22+2*i));// clock symbol
			sprintf(cline+6,"%c",(0x23+2*i));// clock symbol
			sprintf(cline+7,"  ");
			sprintf(cline+9,"%c",(0x3b+i));// thermometer symbol
			sprintf(cline+10,"  ");
			sprintf(cline+12,"%c",(0x3e+i));// candle symbol
			sprintf(cline+13,"%c",(0x3e+i));// candle symbol
			print_lcd(2, cline);
			DrvSYS_Delay(500000);
		}
	}
	clr_all_panel();
	while(1)
	{
		if (key==timer_fun) {key=0; set_timer=true; timer();}
		if (key==temp_fun) {key=0; set_temp=true; temperature();}
		if (key==shabat_fun) {key=0; set_shabat=true; Shabat();}
		if (key==set_fun) {key=0; Set_clock=true; Set_Clock();}
		//clock & real temperature display	
		sprintf(cline,"                ");
		sprintf(cline,"%c",0x60);// bluetooth symbol
		sprintf(cline+1," HC05");
		if(sel_timer==true){sprintf(cline+6,"        "); sprintf(cline+14,"%c",(0x22+2*s)); sprintf(cline+15,"%c",(0x23+2*s));}// clock symbol
		if(sel_temp==true){sprintf(cline+6,"         "); sprintf(cline+15,"%c",(0x3b+s));}// thermometer symbol
		if(sel_shabat==true){sprintf(cline+6,"        "); sprintf(cline+14,"%c",(0x3e+s)); sprintf(cline+15,"%c",(0x3e+s));}// candle symbol
		if(sel_boiler==true)sprintf(cline+6,"        on");
		if(sel_timer==false && sel_temp==false && sel_shabat==false && sel_boiler==false)sprintf(cline+6,"          ");
		print_lcd(0, cline);
		sprintf(clock_s,"Clock:          ");//clock display
		if (hour<10)    sprintf(clock_s+8,"0%d:", hour);
		else            sprintf(clock_s+8,"%2d:", hour);   
		if (min<10)  sprintf(clock_s+11,"0%d:", min);
		else            sprintf(clock_s+11,"%2d:", min);
		if (sec<10)  sprintf(clock_s+14,"0%d", sec);
		else            sprintf(clock_s+14,"%2d", sec);
		print_lcd(1, clock_s);
		sprintf(temp_s,"Temperature:    ");//real temperature display
		if (temp>0 && temp<99)sprintf(temp_s+12,"%d.c", temp);
		print_lcd(2, temp_s);
		if(hour_t==0 && min_t==0 && sec_t==0 && sel_temp==false && sel_shabat==false)
		{
			sprintf(cline,"                ");
			print_lcd(3, cline);
		}
		
		if (hour_t>0 || min_t>0 || sec_t>0)//timer display
		{
			sprintf(time,"Timer:          ");
			if (hour_t<10)    sprintf(time+8,"0%d:", hour_t);
			else            sprintf(time+8,"%2d:", hour_t);   
			if (min_t<10)  sprintf(time+11,"0%d:", min_t);
			else            sprintf(time+11,"%2d:", min_t);
			if (sec_t<10)  sprintf(time+14,"0%d", sec_t);
			else            sprintf(time+14,"%2d", sec_t);
			print_lcd(3, time);
		}
		if (sel_temp==true)//temperature control display
		{
			sprintf(cline,"                ");
			sprintf(cline,"Target temp:%d.c",value_temp);
			print_lcd(3, cline);
		}
		if (sel_shabat==true)//shabat display
		{
			sprintf(cline,"                ");
			sprintf(cline,"shabat: ");
			if (hour_shabat_on<10) sprintf(cline+8,"0%d:", hour_shabat_on);
		else               sprintf(cline+8,"%2d:", hour_shabat_on);   
		if (min_shabat_on<10)  sprintf(cline+11,"0%d:", min_shabat_on);
		else               sprintf(cline+11,"%2d:", min_shabat_on);
			sprintf(cline+14,"00");
			print_lcd(3, cline);
		}
	}
}