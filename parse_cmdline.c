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

// one more than real max, so we can check for excess arguments:
#define MAXARG 10


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
	printf("               clk:     the GPI number of the first encoder contact (0-%d)\n", MAXGPIO);
	printf("               dt:      the GPI number of the second encoder contact (0-%d)\n", MAXGPIO);
        printf("               Depending on 'type', other options must follow:\n\n");
#ifdef HAVE_JACK
	printf("      ...,jack,cc,[ch[,min[,max[,step[,default]]]]]\n");
	printf("               cc:      MIDI continous controller number (0-%d)\n", MAXCC);
	printf("               ch:      MIDI channel (1-16), default 1\n");
	printf("               min:     minimum controller value (0-%d), default 0\n", MAXCCVAL);
	printf("               max:     maximum controller value (0-%d), default %d\n", MAXCCVAL, MAXCCVAL);
	printf("               step:    the step size per 'click'(1-%d), default 1\n", MAXCCVAL);
	printf("               default: the initial value, default is 'min'\n\n"); 
#endif
#ifdef HAVE_ALSA
	printf("      ...,alsa,control[,card[,step]]\n");
	printf("               control: the name of a simple controller in ALSA mixer\n");
	printf("               card:    the name of a sound interface (defaults to hw:0)\n");
	printf("               step: the step size in dB per click, default 3\n\n");
#endif
	printf("      ...,stdout,format[,min[,max[,step[,default]]]]].\n");
	printf("               format:  a string that can contain the special tokens '%%pin%%'\n");
	printf("                        (the pin number) and '%%val%%' (the value)\n");
	printf("               min:     minimum value (%d-%d), default 0\n", INT_MIN, INT_MAX);
	printf("               max:     maximum value (%d-%d), default 100\n", INT_MIN, INT_MAX);
	printf("               step:    the step size per click, default 1\n"); 
	printf("               default:	the initial value, default is 'min'\n\n");
	printf("-s|--switch sw,type...\n");
	printf("               Set up a switch. Again, all following parameters must be\n");
	printf("               separated only by commas:\n");
	printf("               sw:      the GPI pin number of the switch contact (0-%d)\n", MAXGPIO);
        printf("               Depending on 'type', other options must follow:\n\n");
#ifdef HAVE_JACK
	printf("      ...,jack,cc,[ch[,latch[,min[,max[,default]]]]]]\n");
	printf("               cc:      MIDI continous controller number (0-120)\n");
	printf("               ch:      MIDI channel (1-16), default 1\n");
	printf("               latch:   can be 0 (momentary on) or 1 (toggled on/off)\n");
	printf("               min:     controller value when open (0-%d), default 0\n", MAXCCVAL);
	printf("               max:     controller value when closed (0-%d), default %d\n", MAXCCVAL, MAXCCVAL);
	printf("               default: the initial value, default is 'min'\n\n"); 
#endif
#ifdef HAVE_ALSA
	printf("      ...,alsa,control[,card]\n");
	printf("               control: the name of a simple controller in ALSA mixer\n");
	printf("                        (switch will operate the MUTE function)\n");
	printf("               card:    the name of a sound interface (defaults to hw:0)\n\n");
#endif
	printf("      ...,stdout,format[,latch[,min[,max[,default]]]]\n");
	printf("               format:  a string that can contain the special tokens '%%pin%%'\n");
	printf("                        (the pin number) and '%%val%%' (the value)\n");
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
	// always go up to MAXARG, to make sure we overwrite all previous hits with NULL
	for (int k=1; k <= MAXARG; k++) {
		if (config[i] != NULL) i++;
		config[k] = strtok(NULL, ",");
	}
	return i;
}

static int match(char *string1, char *string2) {
	if (strncmp(string1, string2, strlen(string2)) == 0) {
		return 1;
	} else {
		return 0;
	}
}	

int parse_cmdline(int argc, char *argv[])
{
	int c;
	int i;
	char *config[MAXARG];

	control_t* data;

	static struct option long_options[] = {
		{"help", no_argument, 0, 'h'},
		{"verbose", no_argument, 0, 'v'},
		{"rotary", required_argument, 0, 'r'},
		{"switch", required_argument, 0, 's'},
		{0, 0, 0, 0}
	};
	
	while (1) {
		int optind = 0;
		c = getopt_long(argc, argv, "hvr:s:", long_options,
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
		case 'r':
			i = tokenize(optarg, config);
			data = (control_t*) malloc(sizeof(control_t));
			if (data == NULL) {
				ERR("malloc() failed.");
				return EXIT_ERR;
			}
			if (i < 4) {
				ERR("Not enough options for -r.");
				goto error;
			}
			data->pin1 = atoi(config[0]);
			if (data->pin1 < 0 || data->pin1 > MAXGPIO) {
				ERR("clk value out of range.");
				goto error;
			}
			if (controllers[data->pin1] != NULL) {
				ERR("clk pin already assigned.");
				goto error;
			}
			data->pin2 = atoi(config[1]);
			if (data->pin2 < 0 || data->pin2 > MAXGPIO) {
				ERR("dt value of of range.");
				goto error;
			}
			if (controllers[data->pin2] != NULL) {
				ERR("dt pin already assigned.");
				goto error;
			}
			controllers[data->pin1] = data;
			controllers[data->pin2] = data;
			data->type = ROTARY;
#ifdef HAVE_JACK
			if (match(config[2], "jack")) {
				data->target = JACK;
				data->midi_cc = atoi(config[3]);
				if (data->midi_cc < 0 || data->midi_cc > MAXCC) {
					ERR("MIDI CC value out of range.");
					goto error;
				}
				if (config[4] == NULL) {
					data->midi_ch = 0;
				} else {
					data->midi_ch = atoi(config[4]) - 1;
					if (data->midi_ch < 0 || data->midi_ch > MAXMIDICH) {
						ERR("MIDI channel value out of range.");
						goto error;
					}
				}		
				if (config[5] == NULL) {
					data->min = 0;
				} else {
					data->min = atoi(config[5]);
					if (data->min < 0 || data->min > MAXCCVAL) {
						ERR("min value out of range.");
						goto error;
					}
				}
				if (config[6] == NULL) {
					data->max = MAXCCVAL;
				} else {
					data->max = atoi(config[6]);
					if (data->max < 0 || data->min > MAXCCVAL) {
						ERR("max value out of range.");
						goto error;
					}
				}
				if (config[7] == NULL) {
					data->step = 1;
				} else {
					data->step = atoi(config[7]);
					if (data->step < 1 || data->step > MAXCCVAL) {
						ERR("step value out of range.");
						goto error;
					}
				}
				if (config[8] == NULL) {
					data->value = data->min;
				} else {
					data->value = atoi(config[8]);
					if (data->value < data->min || data->value > data->max) {
						ERR("default value out of range.");
						goto error;
					}
				}
				if (config[9] != NULL) {
					ERR("Too many arguments.");
					goto error;
				}
				use_jack = 1;
			} else
#endif
#ifdef HAVE_ALSA			 
			if (match(config[2], "alsa")) {
				data->target = ALSA;
				data->param1 = calloc(sizeof(char), MAXNAME);
				data->param1 = strncpy(data->param1, config[3], MAXNAME);
				data->param2 = calloc(sizeof(char), MAXNAME);
				if (config[4] == NULL) {
					data->param2 = "hw:0";
				} else {
					data->param2 = strncpy(data->param2, config[4], MAXNAME);
				}
				if (config[5] == NULL) {
					data->step = 3;
				} else {
					data->step = atoi(config[5]);
				}
				if (config[6] != NULL) {
					ERR("Too many arguments.");
					goto error;
				}					
			} else
#endif			 
			if (match(config[2], "stdout")) {	
				data->target = STDOUT;
				data->param1 = calloc(sizeof(char), MAXNAME);
				data->param1 = strncpy(data->param1, config[3], MAXNAME);
				// TODO: check for presence of %% tokens instead!
				if (strlen(data->param1) < 1) {
					ERR("format cannot be empty.");
					goto error;					
				}
				if (config[4] == NULL) {
					data->min = 0;
				} else {
					data->min = atoi(config[4]);
				}
				if (config[5] == NULL) {
					data->max = 100;
				} else {
					data->max = atoi(config[5]);
				}
				if (config[6] == NULL) {
					data->step = 1;
				} else {
					data->step = atoi(config[6]);
				}
				if (config[7] == NULL) {
					data->value = data->min;
				} else {
					data->value = atoi(config[7]);
				}
				if (config[8] != NULL) {
					ERR("Too many arguments.");
				}
				DBG("Parsed control type=%d pin1=%d pin2=%d target=%d min=%d max=%d step=%d default=%d.",
					data->type, data->pin1, data->pin2, data->target, data->min, data->max, data->step,
					data->value);
			} else {
				ERR("Unknown type '%s'.", config[2]);
				goto error;
			}			
			break;
		case 's':
			break;
		default:
			ERR("Unknown option.");
			return EXIT_ERR;
		}
	}
	if (argc < 2) {
		ERR("You need to set at least one control.");
		return EXIT_ERR;
	}
	return EXIT_CLEAN;
	error: {
		free(data);
		return EXIT_ERR;
	}
}

