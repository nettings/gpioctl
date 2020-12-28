#!/bin/bash

# test board with four ALPS rotaries with switch
PINS="7 8 12 13 16 17 22 23 24 25 26 27"

for PIN in $PINS ; do
	echo -n "Setting pin [$PIN] to input..."
	raspi-gpio set "$PIN" ip && echo "ok." || echo "failed"
done
echo
for PIN in $PINS ; do
	echo -n "Enabling pull-up resistor for pin [$PIN]..."
	raspi-gpio set "$PIN" pu && echo "ok." || echo "failed."
done

