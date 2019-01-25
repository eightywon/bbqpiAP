# bbqpiAP

blink.service  
  
[unit]  
Description=Monitors wifi connection  
After=multi-user.target  
[Service]  
ExecStart=/home/pi/blink  
WorkingDirectory=/home/pi  
StandardOutput=inherit  
StandardError=inherit  
Restart=always  
  
[Install]  
WantedBy=multi-user.target  
  
  
physical pinouts:  
Button 15, 2 (5v)  
LED 13 (green leg), 11 (red leg), 34 (gnd)  

sudo gcc -o ../blink bbqpiAP.c -lwiringPi -lsqlite3 -liw
