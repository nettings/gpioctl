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
#include "slave_process.h"
#endif

control_t *controller[NCONTROLLERS] = { 0 };

int verbose = 0;
int use_alsa = 0;
int use_jack = 0;
int use_osc = 0;
int use_stdout = 0;

char* osc_url;

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
#ifdef HAVE_OSC
	if (use_osc) {
		shutdown_OSC();
	}
	if (use_slave) {
		shutdown_SLAVE();
	}
#endif
	shutdown_GPIOD();
	exit(0);
}

void update(control_t* c, int delta)
{
	switch (c->type) {
	case ROTARY:
		if (c->target == MASTER) {
			// only send relative changes to slaves
			c->value = delta * c->step;
			break;
		}
#ifdef HAVE_ALSA
		if (c->target == ALSA || c->target == SLAVE) {
			// to avoid loudness jumps, we always re-read the current mixer value
			// in case it got changed by someone else, and then apply a relative
			// change
			c->value = get_ALSA_mixer_value(c);
			// clamp to our range. some mixers have min values of -999999 and max
			// values of +4 or so...
			if (c->value < c->min) c->value = c->min;
			if (c->value > c->max) c->value = c->max;
		}
#endif
		if ((delta < 0 && c->value > c->min)) {
			if (c->value - c->step > c->min) {
				c->value -= c->step;
			} else {
				c->value = c->min;
			}
		} else if ((delta > 0 && c->value < c->max)) {
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
			if (delta == 0)
				return;
			if (c->value > c->min) {
				c->value = c->min;
			} else {
				c->value = c->max;
			}
		} else {
			if (delta == 0) {
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
		update_STDOUT(c);
		break;
#ifdef HAVE_JACK
	case JACK:
		update_JACK(c);
		break;
#endif
#ifdef HAVE_ALSA
	case ALSA:
		update_ALSA(c);
		break;
	case SLAVE:
		update_ALSA(c);
		break;
#endif
#ifdef HAVE_OSC
	case OSC:
		update_OSC(c);
		break;
	case MASTER:
		update_OSC(c);
		break;
#endif
	default:
		ERR("Unknown c->target %d. THIS SHOULD NEVER HAPPEN.",
		    c->target);
	}
}

void handle_gpi(int line, int delta)
{
	control_t *c = controller[line];
	update(c, delta);
}

void handle_osc(control_t *c, int delta) {
	update(c, delta);
}

int main(int argc, char *argv[])
{
	control_t *c;

	int rval = parse_cmdline(argc, argv);
	if (rval == EXIT_USAGE) {
		usage();
		exit(0);
	}
	if (rval != EXIT_CLEAN) {
		exit(rval);
	}
	setup_GPIOD(GPIOD_DEVICE, PROGRAM_NAME, &handle_gpi);
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
	if (use_slave) {
		setup_SLAVE(osc_url, &handle_osc);
	}
#endif
	for (int i = 0; i < NCONTROLLERS; i++) {
		if (controller[i] == NULL)
			continue;
		c = controller[i];
		switch (c->target) {
		case NOTGT:
			// this line is a dt pin for a rotary, without its own handler
			continue;
#ifdef HAVE_ALSA
		case ALSA:
			c->param1 = setup_ALSA_mixer_elem(c->param1);
			// fall-through
#endif
		case JACK:
		case OSC:
		case STDOUT:
		case MASTER:
			switch (c->type) {
			case ROTARY:
				setup_GPIOD_rotary(c->pin1, c->pin2);
				break;
			case SWITCH:
				setup_GPIOD_switch(c->pin1);
				break;
			default:
				ERR("c->type %d can't happen here. BUG?", c->type);
			}
			break;
		case SLAVE:
			c->param1 = setup_ALSA_mixer_elem(c->param1);
			switch (c->type) {
			case ROTARY:
				setup_SLAVE_handler(c->param2, c);
				break;
			case SWITCH:
				setup_SLAVE_handler(c->param2, c);
				break;
			default:
				 ERR("c->type %d can't happen here. BUG?", c->type);
			}
			break;
		default:
			ERR("Unknown c->target %d. THIS SHOULD NEVER HAPPEN.",
			    c->target);
		}
	}

	signal(SIGTERM, &signal_handler);
	signal(SIGINT, &signal_handler);

	start_GPIOD();
#ifdef HAVE_OSC
	if (use_slave) start_SLAVE();
#endif

	sleep(-1);
}
