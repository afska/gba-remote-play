#!/bin/bash

sudo pigpiod > /dev/null 2>&1
sudo python3 monitor.py
