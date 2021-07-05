#!/usr/bin/env python

# monitor.py
# 2016-09-17
# Public Domain

# monitor.py          # monitor all GPIO
# monitor.py 23 24 25 # monitor GPIO 23, 24, and 25

import sys
import time
import pigpio

last = [None]*32
cb = []

def cbf(GPIO, level, tick):
   if last[GPIO] is not None:
      diff = pigpio.tickDiff(last[GPIO], tick)
      print("G={} l={} d={}".format(GPIO, level, diff))
   last[GPIO] = tick

pi = pigpio.pi()

if not pi.connected:
   exit()

if len(sys.argv) == 1:
   G = range(0, 32)
else:
   G = []
   for a in sys.argv[1:]:
      G.append(int(a))
   
for g in G:
   cb.append(pi.callback(g, pigpio.EITHER_EDGE, cbf))

try:
   while True:
      time.sleep(60)
except KeyboardInterrupt:
   print("\nTidying up")
   for c in cb:
      c.cancel()

pi.stop()
