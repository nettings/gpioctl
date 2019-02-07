#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <pthread.h>

#include "globals.h"
#include "parse_cmdline.h"
#include "jack_process.h"
#include "gpiod_process.h"
#include "alsa_process.h"
#include "ringbuffer.h"

controller_t controllers[] = {0};
int verbose = 0;

static void signal_handler(int sig)
{
	fprintf(stderr, "Received signal, terminating.\n");
	shutdown_ALSA();
	shutdown_JACK();
	shutdown_ringbuffer();
	shutdown_gpiod();
}

void jackrot_callback(int line, int val) {
	jack_rotary_t* d = (jack_rotary_t*) controllers[line].data;
	unsigned char msg[MSG_SIZE];
	int nbytes;

	if ((val < 0 && d->counter > 0)) {
		if (d->counter > d->step) {
			d->counter -= d->step;
		} else{
			d->counter = 0;
		}
	} else 	if ((val > 0 && d->counter < MIDI_MAX)) {
		if (d->counter + d->step < MIDI_MAX) {
			d->counter += d->step;
		} else {
			d->counter = MIDI_MAX;
		}
	} else return;
	
	msg[0] = (MIDI_CC << 4) + (d->midi_ch - 1);
	msg[1] = d->midi_cc;
	msg[2] = d->counter;
        ringbuffer_write(msg, MSG_SIZE);
	fprintf(stdout, "JACK:\t<R%d|%d> \t0x%02x%02x%02x\n", line, val, msg[0], msg[1], msg[2]);
}

void jacksw_callback(int line, int val) {
	if (val == 0) return;
	fprintf(stdout, "JACK:<S%d|%d>\n", line, val);
}

void alsarot_callback(int line, int val) {
	amixer_rotary_t* ardata = (amixer_rotary_t*) controllers[line].data; 
	fprintf(stdout, "ALSA:<R%d|%d>\n", line, val);
	alsa_set_mixer(ardata->mixer_scontrol, ardata->step, val);	
}

void alsasw_callback(int line, int val) {
	if (val == 0) return;
	amixer_mute_t* amdata = (amixer_mute_t*) controllers[line].data;
	fprintf(stdout, "ALSA:<S%d|%d>\n", line, val);
	alsa_toggle_mute(amdata->mixer_scontrol);
}

int main(int argc, char *argv[])
{
	jack_rotary_t* jrdata;
	jack_switch_t* jsdata;
	amixer_rotary_t* ardata;
	amixer_mute_t* amdata;

	int rval = parse_cmdline(argc, argv);
	if (rval != EXIT_CLEAN) {
		usage();
		exit(rval);
	}
	
	for (int i=0; i < MAXGPIO; i++) {
		fprintf(stdout, "controllers[%d].type = %d\n", i, controllers[i].type);
		switch (controllers[i].type) {
			case JACKROT:
				jrdata = (jack_rotary_t*) controllers[i].data;
				setup_gpiod_rotary(jrdata->clk, jrdata->dt, &jackrot_callback); 
				break;
			case JACKSW:
				jsdata = (jack_switch_t*) controllers[i].data;
				setup_gpiod_switch(jsdata->sw, &jacksw_callback);
				break;
			case ALSAROT:
				ardata = (amixer_rotary_t*) controllers[i].data;
				setup_gpiod_rotary(ardata->clk, ardata->dt, &alsarot_callback);
				break;
			case ALSASW:
				amdata = (amixer_mute_t*) controllers[i].data;
				setup_gpiod_switch(amdata->sw, &alsasw_callback);
				break;
		}
	}

	setup_ringbuffer();
	setup_ALSA();
	setup_JACK();
	setup_gpiod_handler(GPIOD_DEVICE, JACK_CLIENT_NAME);
	signal(SIGTERM, signal_handler);
	signal(SIGINT, signal_handler);

	sleep(-1);
}
