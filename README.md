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

notes re: adding wifi profile from HTML interface:  
$ sudo ./newAP.sh ssid pass  - adds ssid pass to wpa_supplicant.conf  
$ wpa_cli reconfigure - reloads the updated wpa_supplicant.conf  
$ wpa_cli list_networks - shows networks listed in wpa_supplicant.conf, including newly added:  

Selected interface 'p2p-dev-wlan0'  
network id / ssid / bssid / flags  
0       protovision     any  
1       Division        any  
2       test    any  

$ wpa_cli remove_network 2 - removes network id 2 from running config
$ wpa_cli save_config - updates wpa_supplicant.conf from current running config, including removing deleted network

