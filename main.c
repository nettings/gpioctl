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

#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>

#include "globals.h"
#include "parse_cmdline.h"
#include "gpiod_process.h"
#include "build/config.h"
#include "stdout_process.h"

#ifdef HAVE_JACK
#include "ringbuffer.h"
#include "jack_process.h"
#endif

#ifdef HAVE_ALSA
#include "alsa_process.h"
#endif

control_t* controller[MAXGPIO] = { 0 };

int verbose = 0;
int use_jack = 0;

static void signal_handler(int sig)
{
	NFO("Received signal, terminating.");
#ifdef HAVE_ALSA
	if (use_alsa) {
		shutdown_ALSA_mixer();
	}
#endif
#ifdef HAVE_JACK
	if (use_jack) {
		shutdown_JACK();
		shutdown_ringbuffer();
	}
#endif
	shutdown_gpiod();
	exit(0);
}

static void update_stdout(control_t* c) {
	NFO("STDOUT:\t<%02d|%02d>", c->pin1, c->value);
}

#ifdef HAVE_JACK
static void update_jack(control_t* c) {
	unsigned char msg[MSG_SIZE];
	msg[0] = (MIDI_CC << 4) + (c->midi_ch - 1);
	msg[1] = c->midi_cc;
	msg[2] = c->value;
	ringbuffer_write(msg, MSG_SIZE);
	NFO("JACK:\t<%02d|%02d>\t0x%02x%02x%02x", c->pin1, c->value, msg[0],
	    msg[1], msg[2]);
}
#endif

#ifdef HAVE_ALSA
static void update_alsa(control_t* c, int val) {
	switch (c->type) {
	case ROTARY:
		set_ALSA_volume(c->param1, val * c->step * 100);
		break;
	case SWITCH:
		set_ALSA_mute(c->param1, val);
		break;
	}
	NFO("ALSA:\t<%02d|% 2d>", c->pin1 , c->value);
}
#endif

void handle_gpi(int line, int val) {
	control_t* c = controller[line];
#ifdef HAVE_ALSA
	if (c->target == ALSA) {
		update_alsa(c, val);
		return;
	}
#endif
	switch (c->type) {
	case ROTARY:
		if ((val < 0 && c->value > c->min)) {
			if (c->value - c->step > c->min) {
				c->value -= c->step;
			} else {
				c->value = c->min;
			}
		} else if ((val > 0 && c->value < c->max)) {
			if (c->value + c->step < c->max) {
				c->value += c->step;
			} else {
				c->value = c->max;
			}
		} else return;
		break;
	case SWITCH:
		if (c->toggle) {
			if (val == 0) return;
			if (c->value > c->min) {
				c->value = c->min;
			} else {
				c->value = c->max;
			}
		} else {
			 if (val == 0) {
			 	c->value = c->min;
			} else {
				c->value = c->max;
			}
		}
		break;
	default:
		ERR("Unknown c->type %d. THIS SHOULD NEVER HAPPEN.", c->type);
		break;	
	} 
	switch (c->target) {
	case STDOUT:
		update_stdout(c);
		break;
#ifdef HAVE_JACK
	case JACK:
		update_jack(c);
		break;
#endif
#ifdef HAVE_ALSA
	case ALSA:
		break;
#endif;
	default:
		ERR("Unknown c->target %d. THIS SHOULD NEVER HAPPEN.", c->target);
	}
}



int main(int argc, char *argv[])
{
	control_t *c;
	void* p;

	int rval = parse_cmdline(argc, argv);
	if (rval != EXIT_CLEAN) {
		usage();
		exit(rval);
	}
#ifdef HAVE_ALSA
	if (use_alsa) {
		setup_ALSA_mixer();
	}
#endif
	for (int i = 0; i < MAXGPIO; i++) {
		if (controller[i] == NULL) continue;
		c = controller[i];
		switch (c->type) {
		case ROTARY:
			setup_gpiod_rotary(c->pin1, c->pin2, &handle_gpi);
			break;
		case SWITCH:
			setup_gpiod_switch(c->pin1, &handle_gpi);
			break;
		}
		switch (c->target) {
#ifdef HAVE_JACK
		case JACK:
			break;
#endif
#ifdef HAVE_ALSA
		case ALSA:
			p = setup_ALSA_mixer_elem(c->param1);
			c->param1 = p;
			break;
#endif
		case STDOUT:
			break;
		}
	}

#ifdef HAVE_JACK
	if (use_jack) {
		setup_ringbuffer();
		setup_JACK();
	}
#endif
	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);

	setup_gpiod_handler(GPIOD_DEVICE, JACK_CLIENT_NAME);

	sleep(-1);
}

