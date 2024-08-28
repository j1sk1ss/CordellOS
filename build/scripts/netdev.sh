#!/bin/bash
sudo qemu-system-i386 -vga std -k en-us -m 2047M -hda disk.img -netdev bridge,br=br0,id=net0 -device rtl8139,netdev=net0 -serial stdio
# sudo brctl addbr br0
# sudo systemctl restart dhcpd
# sudo ifconfig tap0 192.168.100.1 netmask 255.255.255.0 up
