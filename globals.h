/*
  gpioctl

  Copyright (C) 2019 Jörn Nettingsmeier

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  You should have received a copy of the GNU General Public License
  along with this program. If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef GLOBALS_H
#define GLOBALS_H

#define PROGRAM_NAME "gpioctl"
#define JACK_PORT_NAME "midi_out"
#define GPIOD_DEVICE "pinctrl-bcm2835"
#define MAXGPIO 64
#define MAXMIDICH 15
#define MAXCCVAL 0x7f
#define MAXCC 119
#define MIDI_CC 0xb
#define MSG_SIZE 3
#define MAXNAME 64
#define ALSA_CARD "default"

#include <stdio.h>
#include "build/config.h"

#ifdef DEBUG
#define DBG(fmt, args...) fprintf(stdout, "%s:%d %s(): " fmt "\n", __FILE__, __LINE__, __func__,  ## args)
#define ERR(fmt, args...) fprintf(stderr, "%s:%d %s(): \x1b[01;31m" fmt "\x1b[0m\n", __FILE__, __LINE__, __func__, ## args)
#else
#define DBG(fmt, args...)
#define ERR(fmt, args...) fprintf(stderr, "\x1b[31m"fmt"\x1b[0m\n", ## args)
#endif

#define NFO(fmt, args...) if (verbose) fprintf(stdout, fmt "\n", ## args);

int verbose;
int use_jack;
int use_alsa;

typedef enum {
	NOCTL,
	AUX,
	ROTARY,
	SWITCH
} control_type_t;

typedef enum {
	NOTGT,
	STDOUT,
	JACK,
	ALSA
} control_target_t;

typedef struct {
	unsigned char pin1;
	unsigned char pin2;
	control_type_t type;
	control_target_t target;
	int min;
	int max;
	int step;
	int toggle;
	unsigned char midi_ch;
	unsigned char midi_cc;
	void *param1;
	void *param2;
	int value;
} control_t;

extern control_t *controller[];

#endif
