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

#include "master_cmdline.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "globals.h"

void help_rotary_MASTER()
{
        printf("    ...,master,url[,step]\n");
        printf("               Set up an network master controller for use with -R.\n");
        printf("               url:     The OSC url of the receiver(s), such as\n");
        printf("                        osc.udp://239.0.2.149:7000\n");
        printf("               step:    the step size per click, default 3\n");
}

int parse_cmdline_rotary_MASTER(control_t * c, char *config[])
{
	c->target = MASTER;
	if (config[3] == NULL) {
		ERR("url cannot be empty.");
		return -1;
	}
	c->param1 = strncpy(c->param1, config[3], MAXNAME);
	snprintf(c->param2, MAXNAME, "/%s/level", PROGRAM_NAME);
	if (config[4] == NULL) {
		c->step = 3;
	} else {
		c->step = atoi(config[4]);
	}
	if (config[5] != NULL) {
		ERR("Too many arguments.");
		return -1;
	}
	// we use four times the usual range of a volume control
	// to avoid ever hitting the limits before the slaves do
	c->min = -200;
	c->max = 200;
	c->value = 0;
	c->step = 3;

	return 0;
}

void help_switch_MASTER()
{
        printf("    ...,master,url\n");
        printf("               Set up a network master controller for use with -S.\n");
        printf("               url:     The OSC url of the receiver(s), such as\n");
        printf("                        osc.udp://239.0.2.149:7000\n");
}

int parse_cmdline_switch_MASTER(control_t * c, char *config[])
{
	c->target = MASTER;
	if (config[2] == NULL) {
		ERR("url cannot be empty.");
		return -1;
	}
	c->param1 = strncpy(c->param1, config[2], MAXNAME);
	snprintf(c->param2, MAXNAME, "/%s/mute", PROGRAM_NAME);
	if (config[3] != NULL) {
		ERR("Too many arguments.");
		return -1;
	}
	c->min = 0;
	c->max = 1;
	c->value = 0;
	c->toggle = 1;
	
	return 0;
}
