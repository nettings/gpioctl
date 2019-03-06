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

#include "gpiod_process.h"
#include <gpiod.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include "globals.h"

#define ACTIVE_HIGH 0
#define ACTIVE_LOW 1
#define FOREVER NULL
#define NEVER 0
#define NOAUX -1
#define MAXNAME 64
// debounce time windows, in ms:
#define GPI_DEBOUNCE_SWITCH 50
#define GPI_DEBOUNCE_ROTARY 50

typedef enum {
	GPI_NOTSET,
	GPI_ROTARY,
	GPI_SWITCH,
	GPI_AUX
} line_type_t;

typedef struct {
	line_type_t type;
	int aux;
	unsigned long ts_last;
	int ts_delta;
} line_t;

static line_t *gpi[MAXGPIO] = { 0 };
static void (*user_callback)();

static unsigned int offsets[MAXGPIO] = { 0 };
static int num_lines = 0;
static int shutdown = 0;

static char consumer[MAXNAME];
static char device[MAXNAME];

static unsigned long msec_stamp(struct timespec t)
{
	return (unsigned long)((t.tv_sec * 1000) + (t.tv_nsec / 1000000));
}

static int handle_event(int event, unsigned int line, const struct timespec *timestamp,
	     void *data)
{
	int clk, dt, sw;
	unsigned long now;

	if (shutdown)
		return GPIOD_CTXLESS_EVENT_CB_RET_STOP;

	now = msec_stamp(*timestamp);
	if ((now - gpi[line]->ts_last) > gpi[line]->ts_delta) {
		// we're not bouncing:
		switch (gpi[line]->type) {
		case GPI_ROTARY:
			clk =
			    (event ==
			     GPIOD_CTXLESS_EVENT_CB_RISING_EDGE) ? 1 : 0;
			dt = gpiod_ctxless_get_value(device, gpi[line]->aux,
						     ACTIVE_HIGH, consumer);
			if (clk != dt) {
				user_callback(line, 1);
			} else {
				user_callback(line, -1);
			}
			break;
		case GPI_SWITCH:
			sw = (event ==
			      GPIOD_CTXLESS_EVENT_CB_FALLING_EDGE) ? 1 : 0;
			user_callback(line, sw);
			break;
		default:
			ERR("No handler for type %d. THIS SHOULD NEVER HAPPEN.",
			    gpi[line]->type);
			return GPIOD_CTXLESS_EVENT_CB_RET_ERR;
			break;
		}
		gpi[line]->ts_last = now;
	}
	return GPIOD_CTXLESS_EVENT_CB_RET_OK;
}

int setup_GPIOD_rotary(int line, int aux)
{
	DBG("Adding rotary on pins %d and %d.", line, aux);
	if (gpi[line] != NULL) {
		ERR("Line %d is already in use: %d.", line, gpi[line]->type);
		return -EBUSY;
	}
	if (gpi[aux] != NULL) {
		ERR("Aux %d is already in use: %d.", aux, gpi[aux]->type);
		return -EBUSY;
	}
	if (line == aux) {
		ERR("Line and Aux line cannot both be %d.", line);
		return -EINVAL;
	}
	gpi[line] = calloc(sizeof(line_t), 1);
	if (gpi[line] == NULL) {
		ERR("calloc() failed.");
		return -ENOMEM;
	}
	gpi[aux] = calloc(sizeof(line_t), 1);
	if (gpi[aux] == NULL) {
		ERR("calloc() failed.");
		return -ENOMEM;
	}
	gpi[line]->type = GPI_ROTARY;
	gpi[aux]->type = GPI_AUX;
	gpi[line]->aux = aux;
	gpi[line]->ts_last = NEVER;
	gpi[line]->ts_delta = GPI_DEBOUNCE_ROTARY;
	return 0;
}

int setup_GPIOD_switch(int line)
{
	DBG("Adding switch on pin %d.", line);
	if (gpi[line] != NULL) {
		ERR("Line %d is already in use: %d.", line, gpi[line]->type);
		return -EBUSY;
	}
	gpi[line] = calloc(sizeof(line_t), 1);
	if (gpi[line] == NULL) {
		ERR("calloc() failed.");
		return -ENOMEM;
	}
	gpi[line]->type = GPI_SWITCH;
	gpi[line]->aux = NOAUX;
	gpi[line]->ts_last = NEVER;
	gpi[line]->ts_delta = GPI_DEBOUNCE_SWITCH;
	return 0;
}

int shutdown_GPIOD()
{
	DBG("Shutting down GPIOD.");
	// FIXME: This won't do anything useful unless all lines are being triggered afterwards.
	// We should provide a poll callback and initiate the shutdown there.
	// Then again, all lines are realeased when the process terminates.
        shutdown = 1;
	return 0;
}

int setup_GPIOD(char *dev, char *cons, void (*callback))
{
	DBG("Setting up GPIOD.");
	strncpy(consumer, cons, MAXNAME);
	strncpy(device, dev, MAXNAME);
	user_callback = callback;
	return 0;
}


int start_GPIOD()
{
	int err = 0;
	DBG("Starting GPIOD handler.");
	for (int line = 0; line < MAXGPIO; line++) {
		if (gpi[line] != NULL && gpi[line]->type != GPI_AUX) {
			offsets[num_lines++] = line;
		}
	}
	errno = 0;
	if (num_lines == 0) {
		DBG("No GPIO pins configured, skipping gpiod event handler.");
		return 0;
	}
	err = gpiod_ctxless_event_loop_multiple(device, offsets, num_lines,
						ACTIVE_HIGH, consumer, FOREVER,
						NULL, &handle_event, NULL);
	if (err != 0) {
		ERR("gpiod_ctxless_event_loop_multple: err = %d, errno = %d (%s).", err,
		    errno, strerror(errno));
		return err;
	}
	return 0;
}
