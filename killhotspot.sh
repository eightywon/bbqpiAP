#!/bin/bash
#ip link set wlan0 down
ip link set $1 down
systemctl stop hostapd
systemctl stop dnsmasq
#ip addr flush dev wlan0
ip addr flush dev $1
#ip link set dev wlan0 up
ip link set dev $1 up
#wpa_supplicant -B -i wlan0 -c /etc/wpa_supplicant/wpa_supplicant.conf >/dev/null 2>&1
wpa_supplicant -B -i $1 -c /etc/wpa_supplicant/wpa_supplicant.conf >/dev/null 2>&1
