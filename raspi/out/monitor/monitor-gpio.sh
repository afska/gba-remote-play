#!/bin/bash

# sudo apt-get install pigpio
sudo pigpiod > /dev/null 2>&1
sudo python3 monitor-gpio.py
