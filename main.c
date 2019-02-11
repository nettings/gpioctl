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

#ifdef HAVE_JACK
#include "ringbuffer.h"
#include "jack_process.h"
#endif

#ifdef HAVE_ALSA
#include "alsa_process.h"
#endif

control_t* controllers[MAXGPIO] = { 0 };

int verbose = 0;
int use_jack = 0;

static void signal_handler(int sig)
{
	NFO("Received signal, terminating.");
#ifdef HAVE_ALSA
	shutdown_ALSA();
#endif
#ifdef HAVE_JACK
	if (use_jack) {
		shutdown_JACK();
		shutdown_ringbuffer();
	}
#endif
	shutdown_gpiod();
}

void stdout_callback(int line, int val) {
	NFO("stdout callback on %d with val=%d.", line, val);
}

#ifdef HAVE_JACK
void jack_callback(int line, int val)
{
	control_t* data = controllers[line];
	unsigned char msg[MSG_SIZE];
	switch (data->type) {
	case ROTARY:
		if ((val < 0 && data->value > 0)) {
			if (data->value > data->step) {
				data->value -= data->step;
			} else {
				data->value = 0;
			}
		} else if ((val > 0 && data->value < MAXCCVAL)) {
			if (data->value + data->step < MAXCCVAL) {
				data->value += data->step;
			} else {
				data->value = MAXCCVAL;
			}
		} else {
			return;
		}
		break;
	case SWITCH:
		if (data->toggle) {
			if (val == 0)
				return;
			data->value = (data->value < data->max) ? data->min : data->max;
		} else {
			data->value = val ? data->min : data->max;
		}
		break;
	default:
		ERR("Unknown data->type %d. THIS SHOULD NEVER HAPPEN.", data->type);
		break;	
	} 
	msg[0] = (MIDI_CC << 4) + (data->midi_ch - 1);
	msg[1] = data->midi_cc;
	msg[2] = data->value;
	ringbuffer_write(msg, MSG_SIZE);
	NFO("JACK:\t<%02d|%02d>\t0x%02x%02x%02x", line, val, msg[0],
	    msg[1], msg[2]);
}
#endif

#ifdef HAVE_ALSA
void alsa_callback(int line, int val)
{
	control_t* data = controllers[line];
	switch (data->type) {
	case ROTARY:
		set_ALSA_volume(data->param1, data->step, val);
		break;
	case SWITCH:
		if (val == 0)
			return;
		data->value = 1 - data->value;
		set_ALSA_mute(data->elem, data->value);
		break;
	}
	NFO("ALSA:\t<%02d|% 2d>", line, val);
}
#endif

static void* cb[3] = {
	&stdout_callback
#ifdef HAVE_JACK
	,&jack_callback
#endif
#ifdef HAVE_ALSA
	,&alsa_callback
#endif
};

int main(int argc, char *argv[])
{
	control_t *data;
	

	int rval = parse_cmdline(argc, argv);
	if (rval != EXIT_CLEAN) {
		usage();
		exit(rval);
	}
#ifdef HAVE_ALSA
	setup_ALSA();
#endif
	for (int i = 0; i < MAXGPIO; i++) {
		if (controllers[i] == NULL) continue;
		data = controllers[i];
		switch (data->type) {
		case ROTARY:
			setup_gpiod_rotary(data->pin1, data->pin2, cb[data->target]);
			break;
		case SWITCH:
			setup_gpiod_switch(data->pin1, cb[data->target]);
			break;
		}
	}

#ifdef HAVE_JACK
	if (use_jack) {
		setup_JACK();
		setup_ringbuffer();
	}
#endif

	setup_gpiod_handler(GPIOD_DEVICE, JACK_CLIENT_NAME);
	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);

	sleep(-1);
}
