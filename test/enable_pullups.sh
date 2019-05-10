#!/bin/bash

# test board with four ALPS rotaries with switch

for PIN in 7 8 12 13 16 17 22 23 24 25 26 27 ; do
	echo -n "Enabling pull-up resistor for pin [$PIN]..."
	gpio -g mode "$PIN" up && echo "ok." || echo "failed."
done

