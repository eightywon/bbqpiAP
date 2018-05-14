#!/bin/bash
wpa_cli terminate >/dev/null 2>&1
#ip addr flush wlan0
ip addr flush $1
#ip link set dev wlan0 down
ip link set dev $1 down
rm -r /var/run/wpa_supplicant >/dev/null 2>&1
#ip link set dev wlan0 down
ip link set dev $1 down
#ip a add 172.16.0.1/24 brd + dev wlan0
ip a add 172.16.0.1/24 brd + dev $1
#ip link set dev wlan0 up
ip link set dev $1 up
systemctl start dnsmasq
systemctl start hostapd
