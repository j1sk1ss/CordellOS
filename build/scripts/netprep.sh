#!/bin/bash
sudo systemctl restart dhcpd
sudo ifconfig tap0 192.168.100.1 netmask 255.255.255.0 up
