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
#include <limits.h>
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

#ifdef HAVE_OSC
#include "osc_process.h"
#endif

control_t *controller[MAXGPIO] = { 0 };

int verbose = 0;
int use_alsa = 0;
int use_jack = 0;
int use_osc = 0;
int use_stdout = 0;

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
#ifdef HAVE_OSS
	if (use_oss) {
		shutdown_OSS();
	}
#endif
	shutdown_gpiod();
	exit(0);
}

void handle_gpi(int line, int val)
{
	control_t *c = controller[line];
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
		} else
			return;
		break;
	case SWITCH:
		if (c->toggle) {
			if (val == 0)
				return;
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
		update_alsa(c, val);
		break;
#endif
#ifdef HAVE_OSC
	case OSC:
		update_OSC(c, val);
		break;
#endif
	default:
		ERR("Unknown c->target %d. THIS SHOULD NEVER HAPPEN.",
		    c->target);
	}
}

int main(int argc, char *argv[])
{
	control_t *c;

	int rval = parse_cmdline(argc, argv);
	if (rval != EXIT_CLEAN) {
		usage();
		exit(rval);
	}

#ifdef HAVE_JACK
	if (use_jack) {
		setup_ringbuffer();
		setup_JACK();
	}
#endif
#ifdef HAVE_ALSA
	if (use_alsa) {
		setup_ALSA_mixer();
	}
#endif
#ifdef HAVE_OSC
	if (use_osc) {
		setup_OSC();
	}
#endif
	for (int i = 0; i < MAXGPIO; i++) {
		if (controller[i] == NULL)
			continue;
		c = controller[i];
		switch (c->type) {
		case ROTARY:
			setup_gpiod_rotary(c->pin1, c->pin2, &handle_gpi);
			break;
		case SWITCH:
			setup_gpiod_switch(c->pin1, &handle_gpi);
			break;
		case AUX:
			// this line is a dt pin for a rotary, without its own handler
			continue;
		default:
			 ERR("Unknown c->type %d. THIS SHOULD NEVER HAPPEN.", c->type);
		}
		switch (c->target) {
#ifdef HAVE_ALSA
		case ALSA:
			void *p = setup_ALSA_mixer_elem(c->param1);
			c->param1 = p;
			break;
#endif
		case JACK:
		case OSC:
		case STDOUT:
			break;
		default:
			 ERR("Unknown c->target %d. THIS SHOULD NEVER HAPPEN.", c->target);
		}
	}

	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);

	setup_gpiod_handler(GPIOD_DEVICE, PROGRAM_NAME);

	sleep(-1);
}

