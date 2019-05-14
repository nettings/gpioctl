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

const char* control_types[] = {
        "NOCTL",
        "AUX",
        "ROTARY",
        "SWITCH"
};

const char* control_targets[] = {
        "NOTGT",
        "ALSA",
        "JACK",
        "OSC",
        "STDOUT",
        "MASTER",
        "SLAVE"
};

static void shutdown(int sig)
{
	NFO("Received signal, terminating.");
#ifdef HAVE_ALSA
	if (use_alsa) {
		shutdown_ALSA();
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
	DBG("update: delta = %d", delta);
	switch (c->type) {
	case ROTARY:
	case AUX:
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
			c->value = get_ALSA_value(c);
			// clamp to our range. some mixers have min values of -999999 and max
			// values of +4 or so...
			if (c->value < c->min) c->value = c->min;
			if (c->value > c->max) c->value = c->max;
			// fader taper:
			if (c->value > -13) c->step = 1;
			else if (c->value > -30) c->step = 2;
			else if (c->value > -41) c->step = 3;
			else if (c->value > -49) c->step = 4;
			else if (c->value > -59) c->step = 5;
			else if (c->value > -68) c->step = 9;
			else if (c->value > -70) c->step = 10;
			else c->step = 20;
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
	NFO("%s% 3d\t-> %s\t% 3d", control_types[c->type], c->pin1, control_targets[c->target], c->value);
}

void handle_gpi(int line, int delta)
{
	// in order to properly debounce both rotary contacts, 
	// aux lines now get their own event handler on the libgpiod side.
	// over here, we must redirect them to their rotary main line.
	// the linear search is ugly. FIXME!
	control_t *c = controller[line];
	if (c->type == AUX) {
		for (int i=0; i < NCONTROLLERS; i++) {
			if (controller[i] == NULL) continue;
			if (controller[i]->pin2 == line) {
				update(controller[i], delta);
				break;
			}
		}
	} else {
		update(c, delta);
	}
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
		setup_ringbuffer(JACK_BUFSIZE);
		setup_JACK();
	}
#endif
#ifdef HAVE_ALSA
	if (use_alsa) {
		setup_ALSA();
	}
#endif
#ifdef HAVE_OSC
	if (use_osc) {
		setup_OSC();
	}
	if (use_slave) {
		if (setup_SLAVE(osc_url, &handle_osc))
			exit(2); // fatal with segfaults down the line
	}
#endif
	for (int i = 0; i < NCONTROLLERS; i++) {
		if (controller[i] == NULL)
			continue;
		c = controller[i];
		switch (c->target) {
/* // can't happen since we gave auxes their own gpio handler
		case NOTGT:
			// this line is a dt pin for a rotary, without its own handler
			continue;
*/
#ifdef HAVE_ALSA
		case ALSA:
			if (c->type == AUX) break;
			c->param1 = setup_ALSA_elem(c->param1);
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
			case AUX:
				// do nothing
				break;
			default:
				ERR("c->type %d can't happen here. BUG?", c->type);
			}
			break;
#ifdef HAVE_ALSA
#  ifdef HAVE_OSC
		case SLAVE:
			c->param1 = setup_ALSA_elem(c->param1);
			setup_SLAVE_handler(c->param2, c);
			break;
#  endif
#endif
		default:
			ERR("Unknown c->target %d. THIS SHOULD NEVER HAPPEN.",
			    c->target);
		}
	}

	signal(SIGTERM, &shutdown);
	signal(SIGINT, &shutdown);
#ifdef HAVE_OSC
	if (use_slave) start_SLAVE(); // this one spawns a thread
#endif
	start_GPIOD(); // this one goes to sleep

	sleep(-1);
}
