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

#include "jack_process.h"
#include <errno.h>
#include <jack/jack.h>
#include <jack/midiport.h>
#include "ringbuffer.h"
#include "globals.h"

jack_client_t *client;
jack_port_t *output_port;

static int process(jack_nframes_t nframes, void *arg)
{
	void *port_buf = jack_port_get_buffer(output_port, nframes);
	unsigned char buffer[3];
	jack_nframes_t time = 0;
	jack_midi_clear_buffer(port_buf);
	while (ringbuffer_read(buffer, MSG_SIZE) == MSG_SIZE) {
		if (jack_midi_event_write(port_buf, time++, buffer, MSG_SIZE)
		    == ENOBUFS) {
			// error handling goes here                     
		}
	}
	return 0;
}

int setup_JACK()
{
	if ((client =
	     jack_client_open(PROGRAM_NAME, JackNoStartServer, NULL)) == 0) {
		ERR("Failed to create client. Is the JACK server running?");
		return -1;
	}
	jack_set_process_callback(client, process, 0);
	output_port =
	    jack_port_register(client, JACK_PORT_NAME, JACK_DEFAULT_MIDI_TYPE,
			       JackPortIsOutput, 0);

	if (jack_activate(client)) {
		ERR("Failed to activate client.");
		return -1;
	}
	return 0;
}

int shutdown_JACK()
{
	jack_client_close(client);
	return 0;
}

void update_jack(control_t * c)
{
	unsigned char msg[MSG_SIZE];
	msg[0] = (MIDI_CC << 4) + (c->midi_ch - 1);
	msg[1] = c->midi_cc;
	msg[2] = c->value;
	ringbuffer_write(msg, MSG_SIZE);
	NFO("JACK:\t<%02d|%02d>\t0x%02x%02x%02x", c->pin1, c->value, msg[0],
	    msg[1], msg[2]);
}
