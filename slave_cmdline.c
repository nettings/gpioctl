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

#include "slave_cmdline.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "globals.h"

static int slave_index = 0;

void help_SLAVE()
{
        printf("-U|--osc-url   URL to listen to, e.g. osc.udp://239.0.2.149:7000\n");
        printf("               This is mandatory if -R or -S are used.\n\n");
        printf("-R|--rotary-slave device,control\n");
        printf("               device:  an ALSA device name, such as hw:1\n");
        printf("               control: an ALSA mixer simple control\n\n");
        printf("-S|--switch-slave device,control\n");
        printf("               device:  an ALSA device name, such as hw:1\n");
        printf("               control: an ALSA mixer simple control (operates MUTE)\n\n");
}

int parse_cmdline_rotary_SLAVE(control_t * c, char *config[])
{
	c->type = ROTARY;
	c->target = SLAVE;
	if (slave_index < MAXSLAVE) {
		c->pin1 = MAXGPIO + slave_index++;
	} else {
		ERR("Too many slaves. Compile-time limit is %d.", MAXSLAVE);
		return -1;
	}
	if (config[0] == NULL) {
		ERR("device must not be empty.");
		return -1;
	}
	c->param2 = strncpy(c->param2, config[0], MAXNAME);
	if (config[1] == NULL) {
		ERR("control must not be empty.");
		return -1;
	}
	c->param1 = strncpy(c->param1, config[1], MAXNAME);
	if (config[2] != NULL) {
		ERR("Too many arguments.");
		return -1;
	}
	c->min = -100;
	c->max = 0;
	c->value = c->min;
	c->step = 1; // times the delta coming from the master
	return 0;
}


int parse_cmdline_switch_SLAVE(control_t * c, char *config[])
{
	c->type = SWITCH;
	c->target = SLAVE;
	if (slave_index < MAXSLAVE) {
		c->pin1 = MAXGPIO + slave_index++;
	} else {
		ERR("Too many slaves. Compile-time limit is %d.", MAXSLAVE);
		return -1;
	}
	if (config[0] == NULL) {
		ERR("device must not be empty.");
		return -1;
	}
	c->param2 = strncpy(c->param2, config[0], MAXNAME);
	if (config[1] == NULL) {
		ERR("control must not be empty.");
		return -1;
	}
	c->param1 = strncpy(c->param1, config[1], MAXNAME);
	if (config[1] != NULL) {
		ERR("Too many arguments.");
		return -1;
	}
	c->min = 0;
	c->max = 1;
	c->value = 0;
	// the master is toggled
	c->toggle = 0;
	
	return 0;
}
