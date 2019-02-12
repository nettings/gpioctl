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
	printf("\n%s handles switches and rotary encoders connected to GPIOs, using the\n", PROGRAM_NAME);
	printf("portable libgpiod kernel interface, to send JACK MIDI CC messages via \n");
	printf("%s:%s, directly interact with an ALSA mixer control, or print formatted\n", PROGRAM_NAME, JACK_PORT_NAME);
	printf("values to stdout.\n");
	printf("We assume GPI pins have a pull-up, so the return should be connected to ground.\n");
	printf("-h|--help      This help.\n");
	printf("-v|--verbose   Print current controller values.\n\n");
	printf("The following options may be specified multiple times. All parameters must be\n");
	printf("separated by commas, no spaces. Parameters in brackets are optional.\n\n");
	printf("-r|--rotary clk,dt,type,...\n");
	printf("               Set up a rotary encoder.\n");
	printf("               clk:     the GPI number of the first encoder contact (0-%d)\n", MAXGPIO);
	printf("               dt:      the GPI number of the second encoder contact (0-%d)\n", MAXGPIO);
	printf("               Depending on 'type', the remaining parameters are:\n\n");
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
	printf("      ...,alsa,control[,step]\n");
	printf("               control: the name of a simple controller in ALSA mixer\n");
	printf("               step: the step size in dB per click, default 3\n\n");
#endif
	printf("    ...,stdout,format[,min[,max[,step[,default]]]]].\n");
	printf("               format:  a string that can contain the special tokens '%%gpi%%'\n");
	printf("                        (the pin number) and '%%val%%' (the value)\n");
	printf("               min:     minimum value (%d - %d), default 0\n", INT_MIN, INT_MAX);
	printf("               max:     maximum value (%d - %d), default 100\n", INT_MIN, INT_MAX);
	printf("               step:    the step size per click, default 1\n");
	printf("               default:	the initial value, default is 'min'\n\n");
	printf("-s|--switch sw,type...\n");
	printf("               Set up a switch.\n");
	printf("               sw:      the GPI pin number of the switch contact (0-%d)\n", MAXGPIO);
	printf("               Depending on 'type', the remaining parameters are:\n\n");
#ifdef HAVE_JACK
	printf("      ...,jack,cc,[ch[,toggle[,min[,max[,default]]]]]\n");
	printf("               cc:      MIDI continous controller number (0-120)\n");
	printf("               ch:      MIDI channel (1-16), default 1\n");
	printf("               toggle:  can be 0 (momentary on) or 1 (toggled on/off)\n");
	printf("               min:     controller value when open (0-%d), default 0\n", MAXCCVAL);
	printf("               max:     controller value when closed (0-%d), default %d\n", MAXCCVAL, MAXCCVAL);
	printf("               default: the initial value, default is 'min'\n\n");
#endif
#ifdef HAVE_ALSA
	printf("      ...,alsa,control\n");
	printf("               control: the name of a simple controller in ALSA mixer\n");
	printf("                        (switch will operate the MUTE function)\n");
#endif
	printf("     ...,stdout,format[,toggle[,min[,max[,default]]]]\n");
	printf("               format:  a string that can contain the special tokens '%%gpi%%'\n");
	printf("                        (the pin number) and '%%val%%' (the value)\n");
	printf("               toggle:  can be 0 (momentary on) or 1 (toggled on/off)\n");
	printf("               min:     minimum value (%d - %d), default 0\n", INT_MIN, INT_MAX);
	printf("               max:     maximum value (%d - %d), default 1\n", INT_MIN, INT_MAX);
	printf("               default:	the start value, default is 'min'\n\n");
	printf("Pin numbers above are hardware GPIO numbers. They do not usually correspond\n");
	printf("to physical pin numbers. For the RPi, check https://pinout.xyz/# and look\n");
	printf("for the Broadcom ('BCM') numbers.\n");
	printf("libgpiod does not know how to control the pull-up/pull-down resistors of your\n");
	printf("GPIO pins. Use a hardware-specific external tool to enable them, or add\n");
	printf("physical pull-ups.\n\n");
	printf("%s is meant to run as a daemon. Use CTRL-C or send a SIGTERM to exit.\n\n", PROGRAM_NAME);
}

static int tokenize(char *argument, char *config[])
{
	int i = 0;
	config[0] = strtok(argument, ",");
	// always go up to MAXARG, to make sure we overwrite all previous hits with NULL
	for (int k = 1; k <= MAXARG; k++) {
		if (config[i] != NULL)
			i++;
		config[k] = strtok(NULL, ",");
	}
	return i;
}

static int match(char *string1, char *string2)
{
	if (strncmp(string1, string2, strlen(string2)) == 0) {
		return 1;
	} else {
		return 0;
	}
}

int parse_cmdline(int argc, char *argv[])
{
	int o;
	int i;
	char *config[MAXARG];

	int ncontrols;

	control_t *c = NULL;
	control_t *d = NULL;

	static struct option long_options[] = {
		{"help", no_argument, 0, 'h'},
		{"verbose", no_argument, 0, 'v'},
		{"rotary", required_argument, 0, 'r'},
		{"switch", required_argument, 0, 's'},
		{0, 0, 0, 0}
	};

	while (1) {
		int optind = 0;
		o = getopt_long(argc, argv, "hvr:s:", long_options, &optind);
		if (o == -1)
			break;
		switch (o) {
		case 'h':
			return EXIT_USAGE;
			break;
		case 'v':
			verbose = 1;
			break;

		case 'r':
			i = tokenize(optarg, config);
			c = (control_t *) calloc(sizeof(control_t), 1);
			d = (control_t *) calloc(sizeof(control_t), 1);
			if (c == NULL || d == NULL) {
				ERR("calloc() failed.");
				goto error;
			}
			if (i < 4) {
				ERR("Not enough options for -r.");
				goto error;
			}
			c->pin1 = atoi(config[0]);
			if (c->pin1 < 0 || c->pin1 > MAXGPIO) {
				ERR("clk value out of range.");
				goto error;
			}
			if (controller[c->pin1] != NULL) {
				ERR("clk pin already assigned.");
				goto error;
			}
			c->pin2 = atoi(config[1]);
			if (c->pin2 < 0 || c->pin2 > MAXGPIO) {
				ERR("dt value of of range.");
				goto error;
			}
			if (controller[c->pin2] != NULL) {
				ERR("dt pin already assigned.");
				goto error;
			}
			controller[c->pin1] = c;
			c->type = ROTARY;
			controller[c->pin2] = d;
			d->type = AUX;
#ifdef HAVE_JACK
			if (match(config[2], "jack")) {
				c->target = JACK;
				c->midi_cc = atoi(config[3]);
				if (c->midi_cc < 0 || c->midi_cc > MAXCC) {
					ERR("MIDI CC value out of range.");
					goto error;
				}
				if (config[4] == NULL) {
					c->midi_ch = 0;
				} else {
					c->midi_ch = atoi(config[4]) - 1;
					if (c->midi_ch < 0
					    || c->midi_ch > MAXMIDICH) {
						ERR("MIDI channel value out of range.");
						goto error;
					}
				}
				if (config[5] == NULL) {
					c->min = 0;
				} else {
					c->min = atoi(config[5]);
					if (c->min < 0 || c->min > MAXCCVAL) {
						ERR("min value out of range.");
						goto error;
					}
				}
				if (config[6] == NULL) {
					c->max = MAXCCVAL;
				} else {
					c->max = atoi(config[6]);
					if (c->max < 0 || c->min > MAXCCVAL) {
						ERR("max value out of range.");
						goto error;
					}
				}
				if (config[7] == NULL) {
					c->step = 1;
				} else {
					c->step = atoi(config[7]);
					if (c->step < 1 || c->step > MAXCCVAL) {
						ERR("step value out of range.");
						goto error;
					}
				}
				if (config[8] == NULL) {
					c->value = c->min;
				} else {
					c->value = atoi(config[8]);
					if (c->value < c->min
					    || c->value > c->max) {
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
				c->target = ALSA;
				c->param1 = calloc(sizeof(char), MAXNAME);
				c->param1 =
				    strncpy(c->param1, config[3], MAXNAME);
				if (config[4] == NULL) {
					c->step = 3;
				} else {
					c->step = atoi(config[4]);
				}
				if (config[5] != NULL) {
					ERR("Too many arguments.");
					goto error;
				}
				// FIXME: this is a dirty hack to avoid running into limits
				// while the ALSA and counter logic don't talk to each other.
				c->min = -10000;
				c->max = 10000;
				c->value = 0;
				use_alsa = 1;
			} else
#endif
			if (match(config[2], "stdout")) {
				c->target = STDOUT;
				c->param1 = calloc(sizeof(char), MAXNAME);
				c->param1 =
				    strncpy(c->param1, config[3], MAXNAME);
				// TODO: check for presence of %% tokens instead!
				if (strlen(c->param1) < 1) {
					ERR("format cannot be empty.");
					goto error;
				}
				if (config[4] == NULL) {
					c->min = 0;
				} else {
					c->min = atoi(config[4]);
				}
				if (config[5] == NULL) {
					c->max = 100;
				} else {
					c->max = atoi(config[5]);
				}
				if (config[6] == NULL) {
					c->step = 1;
				} else {
					c->step = atoi(config[6]);
				}
				if (config[7] == NULL) {
					c->value = c->min;
				} else {
					c->value = atoi(config[7]);
				}
				if (config[8] != NULL) {
					ERR("Too many arguments.");
					goto error;
				}
				DBG("Parsed control type=%d pin1=%d pin2=%d target=%d min=%d max=%d step=%d default=%d.", c->type, c->pin1, c->pin2, c->target, c->min, c->max, c->step, c->value);
			} else {
				ERR("Unknown type '%s'.", config[2]);
				goto error;
			}
			ncontrols++;
			break;

		case 's':
			i = tokenize(optarg, config);
			c = (control_t *) calloc(sizeof(control_t), 1);
			if (c == NULL) {
				ERR("calloc() failed.");
				goto error;
			}
			if (i < 3) {
				ERR("Not enough options for -s.");
				goto error;
			}
			c->pin1 = atoi(config[0]);
			if (c->pin1 < 0 || c->pin1 > MAXGPIO) {
				ERR("sw value out of range.");
				goto error;
			}
			if (controller[c->pin1] != NULL) {
				ERR("sw pin already assigned.");
				goto error;
			}
			controller[c->pin1] = c;
			c->type = SWITCH;
#ifdef HAVE_JACK
			if (match(config[1], "jack")) {
				c->target = JACK;
				c->midi_cc = atoi(config[2]);
				if (c->midi_cc < 0 || c->midi_cc > MAXCC) {
					ERR("MIDI CC value out of range.");
					goto error;
				}
				if (config[3] == NULL) {
					c->midi_ch = 0;
				} else {
					c->midi_ch = atoi(config[3]) - 1;
					if (c->midi_ch < 0
					    || c->midi_ch > MAXMIDICH) {
						ERR("MIDI channel value out of range.");
						goto error;
					}
				}
				if (config[4] == NULL) {
					c->toggle = 0;
				} else {
					c->toggle = atoi(config[4]);
					if (c->toggle != 0 && c->toggle != 1) {
						ERR("toggle must be 0 or 1.");
						goto error;
					}
				}
				if (config[5] == NULL) {
					c->min = 0;
				} else {
					c->min = atoi(config[5]);
					if (c->min < 0 || c->min > MAXCCVAL) {
						ERR("min value out of range.");
						goto error;
					}
				}
				if (config[6] == NULL) {
					c->max = MAXCCVAL;
				} else {
					c->max = atoi(config[6]);
					if (c->max < 0 || c->max > MAXCCVAL) {
						ERR("max value out of range.");
						goto error;
					}
				}
				if (config[7] == NULL) {
					c->value = c->min;
				} else {
					c->value = atoi(config[7]);
					if (c->value < c->min
					    || c->value > c->max) {
						ERR("default value out of range.");
						goto error;
					}
				}
				if (config[8] != NULL) {
					ERR("Too many arguments.");
					goto error;
				}
				use_jack = 1;
			} else
#endif
#ifdef HAVE_ALSA
			if (match(config[1], "alsa")) {
				c->target = ALSA;
				c->param1 = calloc(sizeof(char), MAXNAME);
				c->param1 =
				    strncpy(c->param1, config[2], MAXNAME);
				if (config[3] != NULL) {
					ERR("Too many arguments.");
					goto error;
				}
				c->min = 0;
				c->max = 1;
				c->value = 0;
				c->toggle = 1;
				use_alsa = 1;
			} else
#endif
			if (match(config[1], "stdout")) {
				c->target = STDOUT;
				c->param1 = calloc(sizeof(char), MAXNAME);
				c->param1 =
				    strncpy(c->param1, config[2], MAXNAME);
				// TODO: check for presence of %% tokens instead!
				if (strlen(c->param1) < 1) {
					ERR("format cannot be empty.");
					goto error;
				}
				if (config[3] == NULL) {
					c->toggle = 0;
				} else {
					c->toggle = atoi(config[3]);
				}
				if (config[4] == NULL) {
					c->min = 0;
				} else {
					c->min = atoi(config[4]);
				}
				if (config[5] == NULL) {
					c->max = 1;
				} else {
					c->max = atoi(config[5]);
				}
				if (config[6] == NULL) {
					c->value = c->min;
				} else {
					c->value = atoi(config[6]);
				}
				if (config[7] != NULL) {
					ERR("Too many arguments.");
					goto error;
				}
				DBG("Parsed control type=%d pin1=%d pin2=%d target=%d min=%d max=%d step=%d default=%d.", c->type, c->pin1, c->pin2, c->target, c->min, c->max, c->step, c->value);
			} else {
				ERR("Unknown type '%s'.", config[2]);
				goto error;
			}
			ncontrols++;
			break;

		default:
			ERR("Unknown option.");
			return EXIT_ERR;
		}
	}
	if (ncontrols == 0) {
		ERR("You need to set at least one control.");
		return EXIT_ERR;
	}
	return EXIT_CLEAN;
 error:{
		if (c != NULL)
			free(c);
		if (d != NULL)
			free(d);
		return EXIT_ERR;
	}
}
