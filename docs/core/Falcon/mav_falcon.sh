#!/bin/bash
export PATH=$PATH:/bin:/sbin:/usr/bin:/usr/local/bin:/root/.local/bin
export HOME=/root
sleep 5
#mkdir -p /data/logs
#mavproxy.py --master=:14550 --aircraft=script/gtest /data
#/root/.local/bin/mavproxy.py --master=:14550 --aircraft /data
mavproxy.py --master /dev/ttySAC0 --baudrate 57600 --aircraft /data --out 192.168.168.11:14550

