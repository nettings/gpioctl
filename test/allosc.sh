#!/bin/bash

# test board with four ALPS rotaries with switch

../build/gpioctl\
	 -r 27,17,osc,osc.udp://239.0.0.254:3000,/one/dial,0,100,1,0 \
	 -r 23,24,osc,osc.udp://239.0.0.254:3000,/two/dial,0,100,1,0 \
	 -r 8,7,osc,osc.udp://239.0.0.254:3000,/three/dial,0,100,1,0 \
	 -r 12,16,osc,osc.udp://239.0.0.254:3000,/four/dial,0,100,1,0 \
	 -s 13,osc,osc.udp://239.0.0.254:3000,/one/switch,0,0,1 \
	 -s 22,osc,osc.udp://239.0.0.254:3000,/two/switch,0,0,1 \
	 -s 25,osc,osc.udp://239.0.0.254:3000,/three/switch,0,0,1 \
	 -s 26,osc,osc.udp://239.0.0.254:3000,/four/switch,0,0,1
