#include "parse_cmdline.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "globals.h"

#define MAXARG 5

void usage()
{
	printf
	    ("Handles switches and rotary encoders connected to GPIOs using the portable libgpiod kernel interface.\n");
	printf
	    ("Optionally creates JACK MIDI CC messages at %s:%s.\n", JACK_CLIENT_NAME, JACK_PORT_NAME);
	printf
	    ("All pins are pulled up, so the return connectors be connected to ground.\n\n");
	printf("-h|--help                  This help.\n");
	printf("-v|--verbose               Print current controller values.\n");
	printf
	    ("-J|--jack-rotary clk,dt,midi_ch,midi_cc,step\n");
	printf
	    ("-A|--amixer-rotary clk,dt,mixer_scontrol,step\n");
	printf
	    ("-j|--jack-switch sw,midi_ch,midi_cc,toggled\n");
	printf
	    ("-a|--amixer-mute sw,mixer_scontrol\n");
}

static int tokenize(char* argument, char* config[]) {
	int i=0;
	config[0] = strtok(argument, ",");
	while (config[i] != NULL) {
		//fprintf(stdout, "config[%d] = %s\n", i, config[i]);
		i++;
		if (i > 4) break;
		config[i] = strtok(NULL, ",");
	}
	return i;
}

int parse_cmdline(int argc, char *argv[])
{
	int c;
	int i;
	int line;
	char* config[MAXARG] = {0};
	jack_rotary_t* jrdata;
	amixer_rotary_t* ardata;
	jack_switch_t* jsdata;
	amixer_mute_t* amdata;
	while (1) {
		static struct option long_options[] = {
			{"help", no_argument, 0, 'h'},
			{"verbose", no_argument, 0, 'v'},
			{"jack-rotary", required_argument, 0, 'J'},
			{"amixer-rotary", required_argument, 0, 'A'},
			{"jack-switch", required_argument, 0, 'j'},
			{"amixer-mute", required_argument, 0, 'a'},
			{0, 0, 0, 0}
		};
		int optind = 0;

		c = getopt_long(argc, argv, "hvJ:A:j:a:", long_options,
				&optind);
		if (c == -1)
			break;
		switch (c) {
			case 'h':
				return EXIT_USAGE;
				break;
			case 'v':
				verbose = 1;
				break;
			case 'J':
				i = tokenize(optarg, config);
				if (i != 5) {
					fprintf(stderr, "-J needs exactly 5 options.\n");
					return EXIT_ERR;
				}
				jrdata = (jack_rotary_t *)malloc(sizeof(jack_rotary_t));
				line = jrdata->clk = atoi(config[0]);
				controllers[line].type = JACKROT;
				controllers[line].data = jrdata;
				jrdata->dt = atoi(config[1]);
				jrdata->midi_ch = atoi(config[2]);
				jrdata->midi_cc = atoi(config[3]);
				jrdata->step = atoi(config[4]); 
				break;
			case 'A':
				i = tokenize(optarg, config);
				if (i != 4) {
					fprintf(stderr, "-A needs exactly 4 options.\n");
					return EXIT_ERR;
				}
				ardata = (amixer_rotary_t *)malloc(sizeof(amixer_rotary_t));
				line = ardata->clk = atoi(config[0]);
				controllers[line].type = ALSAROT;
				controllers[line].data = ardata;
				ardata->dt = atoi(config[1]);
				strncpy(ardata->mixer_scontrol, config[2], MAXNAME);
				ardata->step = atoi(config[3]);
				break;
			case 'j':
				i = tokenize(optarg, config);
				if (i != 4) {
					fprintf(stderr, "-j needs exactly 4 options.\n");
					return EXIT_ERR;
				}
				jsdata = (jack_switch_t *)malloc(sizeof(jack_rotary_t));
				line = jsdata->sw = atoi(config[0]);
				controllers[line].type = JACKSW;
				controllers[line].data = jsdata;
				jsdata->midi_ch = atoi(config[1]);
				jsdata->midi_cc = atoi(config[2]);
				jsdata->toggled = atoi(config[3]);
				break;
			case 'a':
				i = tokenize(optarg, config);
				if (i != 2) {
					fprintf(stderr, "-a needs exactly 2 options.\n");
					return EXIT_ERR;
				}
				amdata = (amixer_mute_t *)malloc(sizeof(amixer_mute_t));
				line = amdata->sw = atoi(config[0]);
				controllers[line].type = ALSASW;
				controllers[line].data = amdata;
				strncpy(amdata->mixer_scontrol, config[1], MAXNAME);
				break;
		}
	}
	return EXIT_CLEAN;
}
