#!/bin/bash
ip link set wlan0 down
systemctl stop hostapd
systemctl stop dnsmasq
ip addr flush dev wlan0
ip link set dev wlan0 up
wpa_supplicant -B -i wlan0 -c /etc/wpa_supplicant/wpa_supplicant.conf >/dev/null 2>&1
