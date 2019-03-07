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
#include "stdout_cmdline.h"

#ifdef HAVE_JACK
#include "jack_cmdline.h"
#endif

#ifdef HAVE_ALSA
#include "alsa_cmdline.h"
#include "slave_cmdline.h"
#endif

#ifdef HAVE_OSC
#include "osc_cmdline.h"
#include "master_cmdline.h"
#endif

void usage()
{
	printf("\n%s handles switches and rotary encoders connected to GPIOs, using the\n", PROGRAM_NAME);
	printf("portable libgpiod kernel interface, to send text messages to /dev/stdout.\n");
	printf("If enabled at build time, you can also send JACK MIDI CC messages,\n");
	printf("OSC messages, or directly interact with an ALSA mixer control.\n");
	printf("We assume GPI pins have a pull-up, so the return should be connected to ground.\n\n");
	printf("-h|--help      This help.\n");
	printf("-v|--verbose   Print current controller values.\n\n");
	printf("The following options may be specified multiple times. All parameters must be\n");
	printf("separated by commas, no spaces. Parameters in brackets are optional.\n\n");
	printf("-r|--rotary clk,dt,type,...\n");
	printf("               Set up a rotary encoder.\n");
	printf("               clk:     the GPI number of the first encoder contact (0-%d)\n", MAXGPIO - 1);
	printf("               dt:      the GPI number of the second encoder contact (0-%d)\n", MAXGPIO - 1);
	printf("               Depending on 'type', the remaining parameters are:\n\n");
#ifdef HAVE_JACK
	help_rotary_JACK();
	printf("\n");
#endif
#ifdef HAVE_ALSA
	help_rotary_ALSA();
	printf("\n");
#endif
#ifdef HAVE_OSC
	help_rotary_OSC();
	printf("\n");
	help_rotary_MASTER();
	printf("\n");
#endif
	help_rotary_STDOUT();
	printf("\n");
	printf("-s|--switch sw,type...\n");
	printf("               Set up a switch.\n");
	printf("               sw:      the GPI pin number of the switch contact (0-%d)\n", MAXGPIO - 1);
	printf("               Depending on 'type', the remaining parameters are:\n\n");
#ifdef HAVE_JACK
	help_switch_JACK();
	printf("\n");
#endif
#ifdef HAVE_ALSA
	help_switch_ALSA();
	printf("\n");
#endif
#ifdef HAVE_OSC
	help_switch_OSC();
	printf("\n");
	help_switch_MASTER();
	printf("\n");
#endif
	help_switch_STDOUT();
	printf("\n");
#ifdef HAVE_OSC
#  ifdef HAVE_ALSA
	help_SLAVE();
#  endif
#endif
	printf("Pin numbers above are hardware GPIO numbers. They do not usually correspond\n");
	printf("to physical pin numbers. For the RPi, check https://pinout.xyz/# and look\n");
	printf("for the Broadcom ('BCM') numbers.\n");
	printf("libgpiod does not know how to control the pull-up/pull-down resistors of your\n");
	printf("GPIO pins. Use a hardware-specific external tool to enable them, or add\n");
	printf("physical pull-ups.\n\n");
	printf("%s is meant to run as a daemon. Use CTRL-C or send a SIGTERM to exit.\n\n",
	     PROGRAM_NAME);
}

static void debugmsg(control_t * c)
{
	DBG("Parsed control %s(%d|%d)->%s [%d..%d] step=%d toggle=%d midi_ch=%d midi_cc=%d param1=%s param2=%s value=%d",
	    control_types[c->type], c->pin1, c->pin2, control_targets[c->target], c->min, c->max, c->step, c->toggle, c->midi_ch, c->midi_cc,
	    (c->param1 == NULL) ? "''" : (char *)c->param1, (c->param2 == NULL) ? "''" : (char *)c->param2, c->value);
}

static int tokenize(char *argument, char *config[])
{
        int i = 0;
	if (argument == NULL) return 0;
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

	int ncontrols = 0;
	control_t *c;
	control_t *d;

	static struct option long_options[] = {
		{"help", no_argument, 0, 'h'},
		{"verbose", no_argument, 0, 'v'},
		{"rotary", required_argument, 0, 'r'},
		{"switch", required_argument, 0, 's'},
		{"slave-rotary", required_argument, 0, 'R'},
		{"slave-switch", required_argument, 0, 'S'},
		{0, 0, 0, 0}
	};

	while (1) {
		int optind = 0;
		c = NULL;
		d = NULL;
		o = getopt_long(argc, argv, ":hvr:s:U:R:S:", long_options, &optind);
		if (o == -1)
			break;
		i = tokenize(optarg, config);
		switch (o) {
		case 'h':
			return EXIT_USAGE;
		case 'v':
			verbose = 1;
			continue; // skip controls update at end
		case 'r':
			c = calloc(sizeof(control_t), 1);
			d = calloc(sizeof(control_t), 1);
			if (c == NULL || d == NULL) {
				ERR("calloc() failed.");
				goto error;
			}
			if (i < 3) {
				ERR("Not enough options for -r.");
				goto error;
			}
			c->pin1 = atoi(config[0]);
			if (c->pin1 < 0 || c->pin1 >= MAXGPIO) {
				ERR("clk value out of range.");
				goto error;
			}
			if (controller[c->pin1] != NULL) {
				ERR("clk pin already assigned.");
				goto error;
			}
			c->pin2 = atoi(config[1]);
			if (c->pin2 < 0 || c->pin2 >= MAXGPIO) {
				ERR("dt value of of range.");
				goto error;
			}
			if (controller[c->pin2] != NULL) {
				ERR("dt pin already assigned.");
				goto error;
			}
			controller[c->pin1] = c;
			c->type = ROTARY;
			c->param1 = calloc(sizeof(char), MAXNAME);
			c->param2 = calloc(sizeof(char), MAXNAME);
			if (c->param1 == NULL || c->param2 == NULL) {
				ERR("calloc() failed.");
				goto error;
			}
#ifdef HAVE_JACK
			if (match(config[2], "jack")) {
				if (parse_cmdline_rotary_JACK(c, config))
					goto error;
				use_jack = 1;
			} else
#endif
#ifdef HAVE_ALSA
			if (match(config[2], "alsa")) {
				if (parse_cmdline_rotary_ALSA(c, config))
					goto error;
				use_alsa = 1;
			} else
#endif
#ifdef HAVE_OSC
			if (match(config[2], "osc")) {
				if (parse_cmdline_rotary_OSC(c, config))
					goto error;
				use_osc = 1;
			} else
			if (match(config[2], "master")) {
				if (parse_cmdline_rotary_MASTER(c, config))
					goto error;
				use_osc = 1;
			} else
#endif
			if (match(config[2], "stdout")) {
				if (parse_cmdline_rotary_STDOUT(c, config))
					goto error;
				use_stdout = 1;
			} else {
				ERR("Unknown type '%s'.", config[2]);
				goto error;
			}
			controller[c->pin2] = d;
			d->type = AUX;
			d->target = c->target;
			d->min = c->min;
			d->max = c->max;
			d->step = c->step;
			d->toggle = c->toggle;
			d->midi_ch = c->midi_ch;
			d->midi_cc = c->midi_cc;
			d->param1 = calloc(sizeof(char), MAXNAME);
			d->param2 = calloc(sizeof(char), MAXNAME);
			if (c->param1 == NULL || c->param2 == NULL) {
				ERR("calloc() failed.");
				goto error;
			}
			strncpy(d->param1, c->param1, MAXNAME);
			strncpy(d->param2, c->param2, MAXNAME);
			d->value = c->value;
			break;

		case 's':
			c = calloc(sizeof(control_t), 1);
			if (c == NULL) {
				ERR("calloc() failed.");
				goto error;
			}
			if (i < 2) {
				ERR("Not enough options for -s.");
				goto error;
			}
			c->pin1 = atoi(config[0]);
			if (c->pin1 < 0 || c->pin1 >= MAXGPIO) {
				ERR("sw value out of range.");
				goto error;
			}
			if (controller[c->pin1] != NULL) {
				ERR("sw pin already assigned.");
				goto error;
			}
			controller[c->pin1] = c;
			c->type = SWITCH;
			c->param1 = calloc(sizeof(char), MAXNAME);
			c->param2 = calloc(sizeof(char), MAXNAME);
#ifdef HAVE_JACK
			if (match(config[1], "jack")) {
				if (parse_cmdline_switch_JACK(c, config))
					goto error;
				use_jack = 1;
			} else
#endif
#ifdef HAVE_ALSA
			if (match(config[1], "alsa")) {
				if (parse_cmdline_switch_ALSA(c, config))
					goto error;
				use_alsa = 1;
			} else
#endif
#ifdef HAVE_OSC
			if (match(config[1], "osc")) {
				if (parse_cmdline_switch_OSC(c, config))
					goto error;
				use_osc = 1;
			} else
			if (match(config[1], "master")) {
				if (parse_cmdline_switch_MASTER(c, config))
					goto error;
				use_osc = 1;
			} else
#endif
			if (match(config[1], "stdout")) {
				if (parse_cmdline_switch_STDOUT(c, config))
					goto error;
				use_stdout = 1;
			} else {
				ERR("Unknown type '%s'.", config[2]);
				goto error;
			}
			break;
#ifdef HAVE_OSC
#  ifdef HAVE_ALSA
		case 'U':
			osc_url = calloc(sizeof(char), MAXNAME);
			if (osc_url == NULL) {
				ERR("calloc() failed.");
				goto error;
			}
			if (config[0] == NULL) {
				ERR("osc-url must be set.");
				goto error;
			}
			osc_url = strncpy(osc_url, config[0], MAXNAME);
			continue; // skip controls update at end
		case 'R':
			c = calloc(sizeof(control_t), 1);
			if (c == NULL) {
				ERR("calloc() failed.");
				goto error;
			}
			c->param1 = calloc(sizeof(char), MAXNAME);
			c->param2 = calloc(sizeof(char), MAXNAME);
			if (c->param1 == NULL || c->param2 == NULL) {
				ERR("calloc() failed.");
				goto error;
			}
			if (parse_cmdline_rotary_SLAVE(c, config))
				goto error;
			controller[c->pin1] = c;
			use_slave = 1;
			use_alsa = 1;
			break;
		case 'S':
			c = calloc(sizeof(control_t), 1);
			if (c == NULL) {
				ERR("calloc() failed.");
				goto error;
			}
			c->param1 = calloc(sizeof(char), MAXNAME);
			c->param2 = calloc(sizeof(char), MAXNAME);
			if (c->param1 == NULL || c->param2 == NULL) {
				ERR("calloc() failed.");
				goto error;
			}
			if (parse_cmdline_switch_SLAVE(c, config))
				goto error;
			controller[c->pin1] = c;
			use_slave = 1;
			use_alsa = 1;
			break;
#  endif
#endif
		case ':':
			ERR("Option '-%c' requires an argument.", optopt);
			goto error;
		default:
			ERR("Unknown option -%c.", optopt);
			goto error;
		}
		ncontrols++;
		debugmsg(c);

	}
	if (ncontrols < 1) {
		ERR("You need to set at least one control.");
		goto error;
	}
	if (use_slave && (osc_url == NULL)) {
		ERR("You need to specifiy -U with -R and -S.");
		goto error;
	}
	return EXIT_CLEAN;
 error:{
		if (c != NULL) {
			if (c->param1 != NULL)
				free(c->param1);
			if (c->param2 != NULL)
				free(c->param2);
			free(c);
		}
		if (d != NULL) {
			if (d->param1 != NULL)
				free(d->param1);
			if (d->param2 != NULL)
				free(d->param2);
			free(d);
		}
		printf("Use -h for help.\n");
		return EXIT_ERR;
	}
}
