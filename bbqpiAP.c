#define RLED 17 //physical pin 11, red LED
#define GLED 27 //physical pin 13, green LED
#define BUTTON 22 //physical pin 15, spst switch
#define HI 1
#define LOW 0
#define IP 1
#define AP 2
#define SHUTTING_DOWN 3
#define SHUTDOWN 4
#define UP 1
#define DOWN 2
#define BLINK 1
#define SOLID 2
#define SLOW 350
#define FAST 150
#define IP_TIMEOUT 90
#define HOLD_FOR_SHUTDOWN 7
#define HOLD_FOR_TOGGLE 2
#define WIFI_SCAN 30

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <time.h>
#include <iwlib.h>

sqlite3 *db;

int pressedAt,ledThread,buttonState,okToToggle=1,rc;
char devName[16], *zErrMsg=0;

struct Connection {
	int mode;
	int state;
	int since;
	char ip[100];
	int ledMode;
	int blinkSpeed;
	int color;
};

struct AccessPoint {
	char bssid[18];
	char freq[10];
	char signal[10];
	char flags[200];
	char ssid[33];
};

struct Connection con;
struct AccessPoint ap[1000];

static int callback(void *data, int argc, char **argv, char **colName) {
        int i;
        for(i=0; i<argc; i++) {
        }
        return 0;
}

void getDevName(void) {
	FILE *fp;
	fp=popen("iw dev | awk '$1==\"Interface\"{print $2}' ","r");
	if (fp==NULL) {
		printf("Failed to run command\n");
	}
	while (fgets(devName,sizeof(devName)-1,fp)!=NULL) {
	}
	pclose(fp);
	printf("dev name is %s\n",devName);
	//handle failing to get wlan device name here
}

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

void updateNetworks (unsigned int idx) {
	sqlite3_stmt *stmt;
	char sql[100];
	sprintf(sql,"select * from networks where ssid='%s';",ap[idx].ssid);
	printf("the sql is: %s\n",sql);
	sqlite3_prepare_v2(db,sql,-1,&stmt,NULL);
	int i=0;
	int num_cols=0;
	while (sqlite3_step(stmt) != SQLITE_DONE) {
		num_cols=sqlite3_column_count(stmt);
		for (i = 0; i < num_cols; i++) {
		}
	}
	time_t now=time(NULL);
	char buff[20];
	strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));
	if (i!=0) {
		printf("'%s' already in database, updating last",ap[idx].ssid);
		snprintf(sql,100,"update networks set last='%s',signal='%s' where ssid='%s'",buff,ap[idx].signal,ap[idx].ssid);
		rc=sqlite3_exec(db,sql,callback,0,&zErrMsg);
		if (rc!=SQLITE_OK) {
			printf("update SQL error: %s\n",zErrMsg);
		}
	} else {
		snprintf(sql,100,"insert into networks (ssid,signal,last,secure) values ('%s','%s','%s',%d);",ap[idx].ssid,ap[idx].signal,buff,1);
		//printf("insert sql is '%s'",sql);
		rc=sqlite3_exec(db,sql,callback,0,&zErrMsg);
		if (rc!=SQLITE_OK) {
			printf("insert SQL error: %s\n",zErrMsg);
		}
	}
}

void checkWifi () {
	FILE *fp;
	char res[300], buff[100];
	memset(res,0,strlen(res));
	memset(buff,0,strlen(buff));
	sprintf(buff,"wpa_cli scan -i %s",devName);
	fp=popen(buff,"r");
	if (fp==NULL) {
		printf("Failed to run command\n");
	}

	while (fgets(res,sizeof(res)-1,fp)!=NULL) {
	}
	pclose(fp);

	memset(res,0,strlen(res));
	memset(buff,0,strlen(buff));
	sprintf(buff,"wpa_cli scan_results -i %s",devName);
	fp=popen(buff,"r");
	if (fp==NULL) {
		printf("Failed to run command\n");
	}

	unsigned int count=0;
	fgets(res,sizeof(res)-1,fp);

	for (int i=0;i<1000;i++) {
		memset(ap[i].bssid,0,strlen(ap[i].bssid));
		memset(ap[i].freq,0,strlen(ap[i].freq));
		memset(ap[i].signal,0,strlen(ap[i].signal));
		memset(ap[i].flags,0,strlen(ap[i].flags));
		memset(ap[i].ssid,0,strlen(ap[i].ssid));
	}

	while (fgets(res,sizeof(res)-1,fp)!=NULL) {
		printf("res is %s\n",res);
		char ssids[33][5];
		for (int i=0;i<=4;i++) {
			memset(ssids[i],0,strlen(ssids[i]));
		}
		sscanf(res,"%s%s%s%s%s%s%s%s%s%s",ap[count].bssid,ap[count].freq,ap[count].signal,ap[count].flags,
		       ap[count].ssid,ssids[0],ssids[1],ssids[2],ssids[3],ssids[4]);
		//printf("ssid is %s\nssids0 is %s\nssids1 is %s\nssids2 is %s\nssids3 is %s\nssids4 is %s\n",ap[count].ssid,ssids[0],ssids[1],ssids[2],ssids[3],ssids[4]);
		for (int i=0;i<=4;i++) {
			if (strlen(ssids[i])>0) {
				strcat(ap[count].ssid," ");
				strcat(ap[count].ssid,ssids[i]);
			}
		}
		printf("\nssid: %s\tsignal: %s\t flags: %s\n",ap[count].ssid,ap[count].signal,ap[count].flags);
		updateNetworks(count);
		count++;
	}
	pclose(fp);
	fp=NULL;
}

void iwScan(void) {
	wireless_scan_head head;
	wireless_scan *result;
	iwrange range;
	int sock;
	sock=iw_sockets_open();
	if (iw_get_range_info(sock,"wlan0",&range)<0) {
		printf("Error during iw_get_range_info.\n");
	} else {
		if (iw_scan(sock,"wlan0",range.we_version_compiled,&head)<0) {
			printf("Error during iw_scan.\n");
			fprintf(stdout, "iw_scan: %s\n", strerror(errno));
		} else {
			result=head.result;
			while (NULL!=result) {
				printf("iw_scan ssid: %s\n",result->b.essid);
				//printf("iw_scan sig: %s\n",result->stats.qual);
				result=result->next;
			}
		}
	}
}

void toggleMode(int now) {
	char buff[100];
	printf("toggling, m%d s%d\n",con.mode,con.state);
	if (con.mode==AP) {
		printf("killing AP, going IP\n");
		con.blinkSpeed=SLOW;
		con.ledMode=BLINK;
		con.color=GLED;
		con.mode=IP;
		con.state=DOWN;
		con.since=now;
		sprintf(buff,"/home/pi/killhotspot.sh %s",devName);
		system(buff);
	} else if (con.mode==IP) {
		printf("trying to start AP\n");
		con.blinkSpeed=SLOW;
		con.ledMode=BLINK;
		con.color=RLED;
		con.mode=AP;
		con.state=DOWN;
		con.since=now;
		sprintf(buff,"/home/pi/hotspot.sh %s",devName);
		system(buff);
		//system("/home/pi/hotspot.sh");
	}
}

void readPin () {
	buttonState=digitalRead(BUTTON);
	pressedAt=millis()/1000;
	if (buttonState==LOW) {
		okToToggle=1;
	}
}

PI_THREAD (driveLED) {
	while(con.mode!=SHUTDOWN) {
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
	getDevName();
	ledThread=piThreadCreate (driveLED);

        rc=sqlite3_open("/var/www/html/the.db",&db);
        if (rc!=SQLITE_OK) {
                printf("Can't open db: %s\n",sqlite3_errmsg(db));
        } else  {
                printf("db opened\n");
        }

	while (1) {
		secs=millis()/1000;
		if (buttonState==HI && secs-pressedAt>=HOLD_FOR_SHUTDOWN) {
			con.mode=SHUTTING_DOWN;
			con.blinkSpeed=FAST;
			con.ledMode=BLINK;
			con.color=RLED;
			delay(5000);
			con.mode=SHUTDOWN;
			digitalWrite(RLED,LOW);
			digitalWrite(GLED,LOW);
			system("/home/pi/shutdown.sh");
		} else if (buttonState==HI && secs-pressedAt>=HOLD_FOR_TOGGLE && okToToggle==1) {
			okToToggle=0;
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
			con.blinkSpeed=SLOW;
			con.color=RLED;
			con.ledMode=BLINK;
			checkAP();
		} else if (con.mode==AP && con.state==UP) {
			con.ledMode=SOLID;
			con.color=RLED;
		}

		if (secs%WIFI_SCAN==0) {
			checkWifi();
		}

		if (secs%25==0) {
			iwScan();
		}

		fflush(stdout);
		delay(1000);
	}
}
