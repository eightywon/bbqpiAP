#define RLED 23 //physical pin 16, red LED
#define GLED 25 //physical pin 22, green LED
#define BUTTON 24 //physical pin 18, spst switch
#define HI 1
#define LOW 0
#define IP 1
#define AP 2
#define UP 1
#define DOWN 2
#define BLINK 0
#define SOLID 1
#define SLOW 350
#define FAST 150
#define TMR_BLINK 0

#include <pigpio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int pressedAt,isPressed;

struct Connection {
	int mode;
	int state;
	int since;
	char ip[100];
	int ledMode;
	int onFor;
	int offFor;
	int color;
};

struct Connection con;

void checkState (int now) {
	char res[100];
	memset(res,0,strlen(res));
	FILE *fp;
	fp=popen("wpa_cli -i wlan0 status|grep 'ip_address'","r");
	if (fp==NULL) {
		printf("Failed to run command\n");
	}

	while (fgets(res,sizeof(res)-1,fp)!=NULL) {
	}
	pclose(fp);

	if (strlen(res)>0) {
		strncpy(con.ip,res+11,15);
		if (con.mode==AP || con.state==DOWN) {
			con.since=now;
		}
		con.mode=IP;
		con.state=UP;
	} else {
		memset(con.ip,0,strlen(con.ip));
		if (con.state==UP) {
			con.since=now;
		}
		con.state=DOWN;
	}
	fp=NULL;
}

void blinkLED () {
	gpioWrite(con.color,HI);
	gpioDelay(con.onFor*1000);
	gpioWrite(con.color,LOW);
}

void solidLED (int pin) {
	gpioSetTimerFunc(TMR_BLINK,500,NULL);
	gpioWrite(pin,HI);
}

void checkAP () {
	printf("checking AP up?\n");
	char res[100];
	memset(res,0,strlen(res));
	FILE *fp;
	fp=popen("systemctl status hostapd |grep '(running)'","r");
	if (fp==NULL) {
		printf("Failed to run command\n");
	}

	while (fgets(res,sizeof(res)-1,fp)!=NULL) {
	}
	pclose(fp);

	if (strlen(res)>0) {
		printf("AP is up!\n");
		con.mode=AP;
		con.state=UP;
		con.color=RLED;
		gpioSetTimerFunc(TMR_BLINK,con.onFor,NULL);
		//gpioWrite(RLED,HI);
		solidLED(con.color);
		gpioSetTimerFunc(2,2000,NULL);
	}
	fp=NULL;
}

void toggleMode() {
	printf("toggling, m%d s%d\n",con.mode,con.state);
	if (con.mode==AP) {
		printf("killing AP, going IP\n");
		con.mode=IP;
		con.state=DOWN;
		con.color=GLED;
		con.onFor=SLOW;
		con.offFor=SLOW;
		gpioWrite(RLED,LOW);
		//solidLED(con.color);
		gpioSetTimerFunc(TMR_BLINK,con.offFor,blinkLED);
		system("/home/pi/killhotspot.sh");
		printf("ap stopped\n");
		//gpioSetTimerFunc(0,500,NULL);
	} else if (con.mode==IP) {
		printf("trying to start AP\n");
		con.mode=AP;
		con.state=DOWN;
		con.color=RLED;
		con.onFor=FAST;
		con.offFor=FAST;
		gpioWrite(GLED,LOW);
		gpioSetTimerFunc(TMR_BLINK,con.offFor,blinkLED);
		gpioSetTimerFunc(2,500,checkAP);
		system("/home/pi/hotspot.sh");
		//printf("ap started\n");
		//gpioSetTimerFunc(0,500,NULL);
	}
	printf("toggled, m%d s%d\n",con.mode,con.state);
}

void checkPressed(void) {
	if (isPressed==HI) {
		toggleMode();
		gpioSetTimerFunc(1,2000,NULL);
	}
}

void readPin (int gpio,int pin_state,uint32_t tick) {
        int secs,mics;
        gpioTime(PI_TIME_RELATIVE,&secs,&mics);
        isPressed=pin_state;

        if (pin_state==HI) {
                pressedAt=secs;
                gpioSetTimerFunc(1,2000,checkPressed);
        } else {
                gpioSetTimerFunc(1,2000,NULL);
                pressedAt=0;
        }
}

int main (int arg,char **argv ) {

	int secs,mics;

	if (gpioInitialise()<0) {
		printf("Failed to start gpio on BCM PIN %d\n",RLED);
		return 1;
	}
	printf("Starting on BCM PIN %d\n",RLED);

        if (gpioSetAlertFunc(BUTTON,readPin)>0) {
                printf("Failed to set alert on BCM PIN %d\n",BUTTON);
                return 1;
        }
        printf("Alert set on BCM PIN %d\n",BUTTON);

	gpioSetMode(RLED,PI_OUTPUT);
	gpioSetMode(GLED,PI_OUTPUT);

	con.mode=IP;
	con.state=DOWN;

	while (1) {
		gpioTime(PI_TIME_RELATIVE,&secs,&mics);
		if (con.mode==IP) {
			checkState(secs);
		}
		if (con.mode==IP && con.state==UP) {
			gpioSetTimerFunc(TMR_BLINK,500,NULL);
			//gpioWrite(GLED,HI);
			con.color=GLED;
			solidLED(con.color);
			gpioWrite(RLED,LOW);
			printf("connected: %s (%d)",con.ip,secs);
			//gpioDelay(700000);
		} else if (con.mode==IP && con.state==DOWN) {
			if (secs-con.since>=300) {
				printf("no ip 300 seconds, going AP mode %d\n",secs);
				//gpioWrite(RLED,LOW);
				//gpioWrite(GLED,HI);
				//solidLED(con.color);
				toggleMode();
			} else {
				printf("no connection %d\n",secs);
				con.onFor=SLOW;
				con.offFor=SLOW;
				con.color=GLED;
				gpioSetTimerFunc(TMR_BLINK,con.offFor,blinkLED);
			}
		} else if (con.mode==AP && con.state==DOWN) {
			printf("setting up AP %d\n",secs);
			con.onFor=FAST;
			con.offFor=FAST;
			con.color=RLED;
			gpioSetTimerFunc(TMR_BLINK,con.offFor,blinkLED);
		} else if (con.mode==AP && con.state==UP) {
			gpioSetTimerFunc(TMR_BLINK,con.offFor,NULL);
			con.color=RLED;
			solidLED(con.color);
		}
		fflush(stdout);
		gpioDelay(1000000);
	}
}
