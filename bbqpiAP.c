#define RLED 23 //physical pin 16, red LED
#define GLED 25 //physical pin 22, green LED
#define BUTTON 24 //physical pin 18, spst switch
#define HI 1
#define LOW 0
#define IP 1
#define AP 2
#define UP 1
#define DOWN 2
#define BLINK 1
#define SOLID 2
#define SLOW 350
#define FAST 150
#define IP_TIMEOUT 90

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int pressedAt,ledThread,buttonState;

struct Connection {
	int mode;
	int state;
	int since;
	char ip[100];
	int ledMode;
	int blinkSpeed;
	int color;
};

struct Connection con;

void checkState (int now) {
	FILE *fp;
	char res[100];
	memset(res,0,strlen(res));
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

void checkAP () {
	FILE *fp;
	char res[100];
	memset(res,0,strlen(res));
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
		con.ledMode=SOLID;
	}
	fp=NULL;
}

void toggleMode(int now) {
	printf("toggling, m%d s%d\n",con.mode,con.state);
	if (con.mode==AP) {
		printf("killing AP, going IP\n");
		con.blinkSpeed=SLOW;
		con.ledMode=BLINK;
		con.color=GLED;
		con.mode=IP;
		con.state=DOWN;
		con.since=now;
		system("/home/pi/killhotspot.sh");
	} else if (con.mode==IP) {
		printf("trying to start AP\n");
		con.blinkSpeed=FAST;
		con.ledMode=BLINK;
		con.color=RLED;
		con.mode=AP;
		con.state=DOWN;
		con.since=now;
		system("/home/pi/hotspot.sh");
	}
}

void readPin () {
	buttonState=digitalRead(BUTTON);
	pressedAt=millis()/1000;
}

PI_THREAD (driveLED) {
	while(1) {
		digitalWrite(RLED,LOW);
		digitalWrite(GLED,LOW);
		digitalWrite(con.color,HI);
		delay(con.blinkSpeed);
		if (con.ledMode==BLINK) {
			digitalWrite(con.color,LOW);
			delay(con.blinkSpeed);
		}
	}
}

int main (int arg,char **argv ) {
	int secs;
	wiringPiSetupGpio();
	pinMode(BUTTON,INPUT);
	wiringPiISR(BUTTON,INT_EDGE_BOTH,readPin);
	pinMode(RLED,OUTPUT);
	pinMode(GLED,OUTPUT);
	con.mode=IP;
	con.state=DOWN;
	con.color=GLED;
	con.ledMode=BLINK;
	con.blinkSpeed=SLOW;
	ledThread=piThreadCreate (driveLED);

	while (1) {
		secs=millis()/1000;
		if (buttonState==HI && secs-pressedAt>=2) {
			pressedAt=0;
			toggleMode(secs);
		}
		if (con.mode==IP && con.state==UP) {
			con.color=GLED;
			con.ledMode=SOLID;
			printf("connected: %s (%d)",con.ip,secs);
			checkState(secs);
		} else if (con.mode==IP && con.state==DOWN) {
			if (secs-con.since>=IP_TIMEOUT) {
				printf("no ip %d seconds, going AP mode %d\n",IP_TIMEOUT,secs);
				con.since=secs;
				toggleMode(secs);
			} else {
				printf("no connection %d\n",secs);
				con.blinkSpeed=SLOW;
				con.color=GLED;
				con.ledMode=BLINK;
			}
			checkState(secs);
		} else if (con.mode==AP && con.state==DOWN) {
			con.blinkSpeed=FAST;
			con.color=RLED;
			con.ledMode=BLINK;
			checkAP();
		} else if (con.mode==AP && con.state==UP) {
			con.ledMode=SOLID;
			con.color=RLED;
		}
		fflush(stdout);
		delay(1000);
	}
}
