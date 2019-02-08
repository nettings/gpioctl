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
	    ("\n%s handles switches and rotary encoders connected to GPIOs, using\n",
	     JACK_CLIENT_NAME);
	printf
	    ("the portable libgpiod kernel interface, to create JACK MIDI CC messages\n");
	printf("at %s:%s or directly interact with an ALSA mixer control.\n",
	       JACK_CLIENT_NAME, JACK_PORT_NAME);
	printf
	    ("All GPI pins are pulled up, so the return should be connected to ground.\n\n");
	printf("-h|--help      This help.\n");
	printf("-v|--verbose   Print current controller values.\n");
	printf("-J|--jack-rotary clk,dt,ch,cc,step\n");
	printf
	    ("               Set up a rotary encoder on pins [clk] and [dt], and create MIDI\n");
	printf
	    ("               messages on channel [ch] for CC no. [cc] with step size [step].\n");
	printf("-A|--amixer-rotary clk,dt,name,step\n");
	printf
	    ("               Set up a rotary encoder on pins [clk] and [dt], and control\n");
	printf
	    ("               ALSA mixer element [name] with step size [step].\n");
	printf("               Use 'amixer scontrols' to get a list.\n");
	printf("-j|--jack-switch sw,ch,cc,toggled\n");
	printf
	    ("               Set up a switch on pin [sw], and create MIDI messages on channel\n");
	printf
	    ("               [ch] for CC no. [cc]. If [toggled] is 0, the switch will send\n");
	printf
	    ("               value '127' when pressed, and '0' when released. With [toggled]\n");
	printf
	    ("               at 1, one press will latch it to '127', and the next one will\n");
	printf("               release it to '0'.\n");
	printf("-a|--amixer-mute sw,name\n");
	printf
	    ("               Set up a switch on pin [sw], and toggle the MUTE status on ALSA\n");
	printf("               mixer element [name].\n\n");
	printf("The options [JAja] may be specified multiple times.\n\n");
	printf
	    ("Pin numbers below are hardware GPIO numbers. They do not usually correspond\n");
	printf
	    ("to physical pin numbers. For the RPi, check https://pinout.xyz/# and look\n");
	printf
	    ("for the Broadcom ('BCM') numbers.\n\n");
}

static int tokenize(char *argument, char *config[])
{
	int i = 0;
	config[0] = strtok(argument, ",");
	while (config[i] != NULL) {
		i++;
		if (i > 4)
			break;
		config[i] = strtok(NULL, ",");
	}
	return i;
}

int parse_cmdline(int argc, char *argv[])
{
	int c;
	int i;
	int line;
	char *config[MAXARG] = { 0 };
	jack_rotary_t *jrdata;
	amixer_rotary_t *ardata;
	jack_switch_t *jsdata;
	amixer_mute_t *amdata;

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
				ERR("-J needs exactly 5 options.");
				return EXIT_ERR;
			}
			jrdata =
			    (jack_rotary_t *) malloc(sizeof(jack_rotary_t));
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
				ERR("-A needs exactly 4 options.");
				return EXIT_ERR;
			}
			ardata =
			    (amixer_rotary_t *) malloc(sizeof(amixer_rotary_t));
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
				ERR("-j needs exactly 4 options.");
				return EXIT_ERR;
			}
			jsdata =
			    (jack_switch_t *) malloc(sizeof(jack_rotary_t));
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
				ERR("-a needs exactly 2 options.");
				return EXIT_ERR;
			}
			amdata =
			    (amixer_mute_t *) malloc(sizeof(amixer_mute_t));
			line = amdata->sw = atoi(config[0]);
			controllers[line].type = ALSASW;
			controllers[line].data = amdata;
			strncpy(amdata->mixer_scontrol, config[1], MAXNAME);
			break;
		}
	}
	if (argc < 3) {
		ERR("You need to set at least one control.");
		return EXIT_ERR;
	}
	return EXIT_CLEAN;
}
