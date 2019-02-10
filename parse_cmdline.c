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

#include "parse_cmdline.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <limits.h>
#include "globals.h"
#include "build/config.h"

#define MAXARG 5

void usage()
{
	printf("\n%s handles switches and rotary encoders connected to GPIOs, using the\n", JACK_CLIENT_NAME);
	printf("portable libgpiod kernel interface, to create JACK MIDI CC messages at\n");
	printf("%s:%s or directly interact with an ALSA mixer control.\n", JACK_CLIENT_NAME, JACK_PORT_NAME);
	printf("We assume GPI pins have a pull-up, so the return should be connected to ground.\n");
	printf("-h|--help      This help.\n");
	printf("-v|--verbose   Print current controller values.\n\n");
	printf("The following options may be specified multiple times:\n\n");
	printf("-r|--rotary clk,dt,type,...\n");
	printf("               Set up a rotary encoder. All parameters must be separated\n");
	printf("               by commas, no spaces.\n");
	printf("               clk:     the GPI pin number of the first encoder contact\n");
	printf("               dt:      the GPI pin number of the second encoder contact\n");
        printf("               Depending on 'type', other options must follow:\n\n");
#ifdef HAVE_JACK
	printf("      ...,jack,cc,[ch[,min[,max[,step[,default]]]]]\n");
	printf("               cc:      MIDI continous controller number (0-120)\n");
	printf("               ch:      MIDI channel (1-16), default 1\n");
	printf("               min:     minimum controller value (0-127), default 0\n");
	printf("               max:     maximum controller value (0-127), default 127\n");
	printf("               step:    the step size per 'click'(1-127), default 1\n");
	printf("               default: the initial value, default is 'min'\n\n"); 
#endif
#ifdef HAVE_ALSA
	printf("      ...,alsa,control[,card[,step]]\n");
	printf("               control: the name of a simple controller in ALSA mixer\n");
	printf("               card:    the name of a sound interface (defaults to hw:0)\n");
	printf("               step: the step size in dB per click, default 3\n\n");
#endif
	printf("      ...,stdout,format[,min[,max[,step[,default]]]]].\n");
	printf("               format:  a string that can contain the special tokens '%pin%'\n");
	printf("                        (the pin number) and '%val%' (the value)\n");
	printf("               min:     minimum value (%d-%d), default 0\n", INT_MIN, INT_MAX);
	printf("               max:     maximum value (%d-%d), default 100\n", INT_MIN, INT_MAX);
	printf("               step:    the step size per click, default 1\n"); 
	printf("               default:	the initial value, default is 'min'\n\n");
	printf("-s|--switch sw,type...\n");
	printf("               Set up a switch. Again, all following parameters must be\n");
	printf("               separated only by commas:\n");
	printf("               sw:      the GPI pin number of the switch contact\n");
        printf("               Depending on 'type', other options must follow:\n\n");
#ifdef HAVE_JACK
	printf("      ...,jack,cc,[ch[,latch[,min[,max[,default]]]]]]\n");
	printf("               cc:      MIDI continous controller number (0-120)\n");
	printf("               ch:      MIDI channel (1-16), default 1\n");
	printf("               latch:   can be 0 (momentary on) or 1 (toggled on/off)\n");
	printf("               min:     controller value when open (0-127), default 0\n");
	printf("               max:     controller value when closed (0-127), default 127\n");
	printf("               default: the initial value, default is 'min'\n\n"); 
#endif
#ifdef HAVE_ALSA
	printf("      ...,alsa,control[,card]\n");
	printf("               control: the name of a simple controller in ALSA mixer\n");
	printf("                        (switch will operate the MUTE function)\n");
	printf("               card:    the name of a sound interface (defaults to hw:0)\n\n");
#endif
	printf("      ...,stdout,format[,latch[,min[,max[,default]]]]\n");
	printf("               format:  a string that can contain the special tokens '%pin%'\n");
	printf("                        (the pin number) and '%val%' (the value)\n");
	printf("               latch:   can be 0 (momentary on) or 1 (toggled on/off)\n");
	printf("               min:     minimum value (%d-%d), default 0\n", INT_MIN, INT_MAX);
	printf("               max:     maximum value (%d-%d), default 100\n", INT_MIN, INT_MAX);
	printf("               default:	the start value, default is 'min'\n\n");
	printf("Pin numbers above are hardware GPIO numbers. They do not usually correspond\n");
	printf("to physical pin numbers. For the RPi, check https://pinout.xyz/# and look\n");
	printf("for the Broadcom ('BCM') numbers.\n");
	printf("libgpiod does not know how to control the pull-up/pull-down resistors of your\n");
	printf("GPIO pins. Use a hardware-specific external tool to enable them, or add\n");
	printf("physical pull-ups.\n\n");
	printf("%s is meant to run as a daemon. Use CTRL-C or send a SIGTERM to exit.\n\n", JACK_CLIENT_NAME);
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
#ifdef HAVE_JACK
	jack_rotary_t *jrdata;
	jack_switch_t *jsdata;
#endif
#ifdef HAVE_ALSA
	amixer_rotary_t *ardata;
	amixer_mute_t *amdata;
#endif

	while (1) {
		static struct option long_options[] = {
			{"help", no_argument, 0, 'h'},
			{"verbose", no_argument, 0, 'v'},
#ifdef HAVE_JACK
			{"jack-rotary", required_argument, 0, 'J'},
			{"jack-switch", required_argument, 0, 'j'},
#endif
#ifdef HAVE_ALSA
			{"amixer-rotary", required_argument, 0, 'A'},
			{"amixer-mute", required_argument, 0, 'a'},
#endif
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
#ifdef HAVE_JACK
		case 'J':
			i = tokenize(optarg, config);
			if (i != 5) {
				ERR("-J needs exactly 5 options.");
				return EXIT_ERR;
			}
			jrdata =
			    (jack_rotary_t *) malloc(sizeof(jack_rotary_t));
			if (jrdata == NULL) {
				ERR("malloc() failed.");
				return EXIT_ERR;
			}
			line = jrdata->clk = atoi(config[0]);
			controllers[line].type = JACKROT;
			controllers[line].data = jrdata;
			jrdata->dt = atoi(config[1]);
			jrdata->midi_ch = atoi(config[2]);
			jrdata->midi_cc = atoi(config[3]);
			jrdata->step = atoi(config[4]);
			break;
		case 'j':
			i = tokenize(optarg, config);
			if (i != 4) {
				ERR("-j needs exactly 4 options.");
				return EXIT_ERR;
			}
			jsdata =
			    (jack_switch_t *) malloc(sizeof(jack_rotary_t));
			if (jsdata == NULL) {
				ERR("malloc() failed.");
				return EXIT_ERR;
			}
			line = jsdata->sw = atoi(config[0]);
			controllers[line].type = JACKSW;
			controllers[line].data = jsdata;
			jsdata->midi_ch = atoi(config[1]);
			jsdata->midi_cc = atoi(config[2]);
			jsdata->toggled = atoi(config[3]);
			break;
#endif
#ifdef HAVE_ALSA
		case 'A':
			i = tokenize(optarg, config);
			if (i != 4) {
				ERR("-A needs exactly 4 options.");
				return EXIT_ERR;
			}
			ardata =
			    (amixer_rotary_t *) malloc(sizeof(amixer_rotary_t));
			if (ardata == NULL) {
				ERR("malloc() failed.");
				return EXIT_ERR;
			}
			line = ardata->clk = atoi(config[0]);
			controllers[line].type = ALSAROT;
			controllers[line].data = ardata;
			ardata->dt = atoi(config[1]);
			strncpy(ardata->mixer_scontrol, config[2], MAXNAME);
			ardata->step = atoi(config[3]);
			break;
		case 'a':
			i = tokenize(optarg, config);
			if (i != 2) {
				ERR("-a needs exactly 2 options.");
				return EXIT_ERR;
			}
			amdata =
			    (amixer_mute_t *) malloc(sizeof(amixer_mute_t));
			if (amdata == NULL) {
				ERR("malloc() failed.");
				return EXIT_ERR;
			}
			line = amdata->sw = atoi(config[0]);
			controllers[line].type = ALSASW;
			controllers[line].data = amdata;
			strncpy(amdata->mixer_scontrol, config[1], MAXNAME);
			break;
#endif
		case 'X':
			i = tokenize(optarg, config);
			if (i != 3) {
				ERR("-X needs exactly 3 options.");
				return EXIT_ERR;
			}
			
		default:
			ERR("Unknown option or feature not compiled in.");
			return EXIT_ERR;
		}
	}
	if (argc < 3) {
		ERR("You need to set at least one control.");
		return EXIT_ERR;
	}
	return EXIT_CLEAN;
}
