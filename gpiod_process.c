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
#define GPI_DEBOUNCE_SWITCH 100
#define GPI_DEBOUNCE_ROTARY 20

typedef enum  {
	GPI_AUX = -1,		// used as secondary port for other line, no separate interrupt handler
	GPI_FREE = 0,
	GPI_ROTARY = 1,
	GPI_SWITCH = 2
} line_type_t;

typedef struct {
	line_type_t type;
	int aux;
	unsigned long ts_last;
	int ts_delta;
	int (*cb) ();
} line_t;


static line_t* gpi[MAXGPIO] = { 0 };

static char consumer[MAXNAME];
static char device[MAXNAME];

static unsigned long msec_stamp(struct timespec t)
{
	return (unsigned long)((t.tv_sec * 1000) + (t.tv_nsec / 1000000));
}

int callback(int event, unsigned int line, const struct timespec *timestamp,
	     void *data)
{
	int clk, dt, sw;
	unsigned long now;
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
				gpi[line]->cb(line, 1);
			} else {
				gpi[line]->cb(line, -1);
			}
			break;
		case GPI_SWITCH:
			sw = (event ==
			      GPIOD_CTXLESS_EVENT_CB_FALLING_EDGE) ? 1 : 0;
			gpi[line]->cb(line, sw);
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

int null_callback(int event, unsigned int line, const struct timespec *timeout,
		  void *data)
{
	ERR("This should never be called.");
	return GPIOD_CTXLESS_EVENT_CB_RET_ERR;
}

void setup_gpiod_rotary(int line, int aux, void (*user_callback))
{
	DBG("int line=%d, int aux=%d, void (*user_callback)=%p", line, aux,
	    user_callback);
	if (gpi[line]->type != GPI_FREE) {
		ERR("Line %d is already in use: %d.", line,
		    gpi[line]->type);
		return;
	}
	if (gpi[aux]->type != GPI_FREE) {
		ERR("Aux %d is already in use: %d.", aux, gpi[aux]->type);
		return;
	}
	if (line == aux) {
		ERR("Line and Aux line cannot both be %d.", line);
		return;
	}
	gpi[line]->type = GPI_ROTARY;
	gpi[aux]->type = GPI_AUX;
	gpi[line]->aux = aux;
	gpi[line]->ts_last = NEVER;
	gpi[line]->ts_delta = GPI_DEBOUNCE_ROTARY;
	gpi[line]->cb = user_callback;
}

void setup_gpiod_switch(int line, void (*user_callback))
{
	if (gpi[line]->type != GPI_FREE) {
		ERR("Line %d is already in use: %d.", line,
		    gpi[line]->type);
		return;
	}
	gpi[line]->type = GPI_SWITCH;
	gpi[line]->aux = NOAUX;
	gpi[line]->ts_last = NEVER;
	gpi[line]->ts_delta = GPI_DEBOUNCE_SWITCH;
	gpi[line]->cb = user_callback;
}

void shutdown_gpiod()
{
	gpiod_ctxless_event_loop_multiple(device, NULL, 0, ACTIVE_HIGH,
					  consumer, FOREVER, NULL,
					  &null_callback, NULL);
	for (int line = 0; line < MAXGPIO; line++) {
		gpi[line]->type == GPI_FREE;
	}
}

void setup_gpiod_handler(char *dev, char *cons)
{
	unsigned int offsets[MAXGPIO];
	int num_lines = 0;
	int err = 0;
	for (int line = 0; line < MAXGPIO; line++) {
		if (gpi[line]->type > GPI_FREE) {
			DBG("Added Pin %d in position %d.", line, num_lines);
			offsets[num_lines++] = line;
		}
	}
	strncpy(consumer, cons, MAXNAME);
	strncpy(device, dev, MAXNAME);
	errno = 0;
	err = gpiod_ctxless_event_loop_multiple(device, offsets, num_lines,
						ACTIVE_HIGH, consumer, FOREVER,
						NULL, callback, NULL);
	ERR("gpiod_ctxless_event_loop_multple: err = %d, errno = %d.", err,
	    errno);
}
