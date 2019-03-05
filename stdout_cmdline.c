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

#include "stdout_cmdline.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "globals.h"

void help_rotary_STDOUT()
{
	printf("    ...,stdout,format[,min[,max[,step[,default]]]]].\n");
	printf("               format:  a string that can contain the special tokens '%%gpi%%'\n");
	printf("                        (the pin number) and '%%val%%' (the value)\n");
	printf("               min:     minimum value (%d - %d), default 0\n", INT_MIN, INT_MAX);
	printf("               max:     maximum value (%d - %d), default 100\n", INT_MIN, INT_MAX);
	printf("               step:    the step size per click, default 1\n");
	printf("               default: the initial value, default is 'min'\n");
}

int parse_cmdline_rotary_STDOUT(control_t * c, char *config[])
{
	c->target = STDOUT;
	c->param1 = strncpy(c->param1, config[3], MAXNAME);
	// TODO: check for presence of %% tokens instead!
	if (strlen(c->param1) < 1) {
		ERR("format cannot be empty.");
		return -1;
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
		return -1;
	}
	return 0;
}

void help_switch_STDOUT()
{
	printf("    ...,stdout,format[,toggle[,min[,max[,default]]]]\n");
	printf("               format:  a string that can contain the special tokens '%%gpi%%'\n");
	printf("                        (the pin number) and '%%val%%' (the value)\n");
	printf("               toggle:  can be 0 (momentary on) or 1 (toggled on/off)\n");
	printf("               min:     minimum value (%d - %d), default 0\n", INT_MIN, INT_MAX);
	printf("               max:     maximum value (%d - %d), default 1\n", INT_MIN, INT_MAX);
	printf("               default: the start value, default is 'min'\n");
}

int parse_cmdline_switch_STDOUT(control_t * c, char *config[])
{
	c->target = STDOUT;
	c->param1 = strncpy(c->param1, config[2], MAXNAME);
	// TODO: check for presence of %% tokens instead!
	if (strlen(c->param1) < 1) {
		ERR("format cannot be empty.");
		return -1;
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
		return -1;
	}
	return 0;
}
