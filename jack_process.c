#include "jack_process.h"
#include <stdio.h>
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
	jack_midi_data_t *dest;
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
	jack_nframes_t nframes;
	if ((client =
	     jack_client_open(JACK_CLIENT_NAME, JackNullOption, NULL)) == 0) {
		fprintf(stderr,
			"Failed to create client. Is the JACK server running?");
		return -1;
	}
	jack_set_process_callback(client, process, 0);
	output_port =
	    jack_port_register(client, JACK_PORT_NAME, JACK_DEFAULT_MIDI_TYPE,
			       JackPortIsOutput, 0);
	nframes = jack_get_buffer_size(client);

	if (jack_activate(client)) {
		fprintf(stderr, "Failed to activate client.\n");
		return -1;
	}
}

int shutdown_JACK()
{
	jack_client_close(client);
}
