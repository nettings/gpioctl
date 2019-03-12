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
// debounce time windows, in us:
#define GPI_DEBOUNCE_SWITCH 50
#define GPI_DEBOUNCE_ROTARY 10

typedef enum {
	GPI_NOTSET,
	GPI_ROTARY,
	GPI_SWITCH,
	GPI_AUX
} line_type_t;

typedef struct {
	line_type_t type;
	unsigned int aux;
	unsigned long ts_last;
	int ts_delta;
} line_t;

/* Yay. A bit-wise state machine :)
 *
 * State:
 *   nnnn
 *   |||+----> clk bit
 *   ||+-----> dt bit
 *   |+------> clockwise bit
 *   +-------> outer ring bit
 *
 * The clk and dt bits are set by interrupt handlers.
 * They can change rapidly when the switch hardware
 * bounces.
 *
 * Two rings:
 *  Outer        1100*_    _1000*
 *                /    \  /    \
 *  Inner        /     0x00     \
 *              |      /  \      |
 *            1101  0010  0101  1010
  *             |     |    |     |
 *               \  0011* 0111* /
 *                \    \  /    /
 *                 \___1x11___/ 
 *
 * The starred states will fire a callback.
 * Between two unstarred states, we tolerate bouncing.
 * A starred state cannot bounce back, because we 
 * change rings immediately, so that it cannot trigger 
 * again.
 * We need two firing states per ring, because
 * in the (clk == dt) case, we cannot tell which
 * direction we're going.
 */
 
#define CLK 0x1
#define DT 0x2
#define CLOCKWISE 0x4
#define OUTER 0x8

#define UPDATE(state,mask,value) (state = (state & ~mask) + (value & mask))
#define IS_SET(state,mask) ((state & mask) == mask)
#define IS_UNSET(state,mask) ((~state & mask) == mask)

#define SET(state,mask) (UPDATE(state, mask, ~0))
#define UNSET(state,mask) (UPDATE(state, mask, 0))
#define TOGGLE(state,mask) (state ^= mask)

#define FIRE_INNER(state) (IS_SET(state,(CLK | DT)) && IS_UNSET(state, OUTER))
#define FIRE_OUTER(state) (IS_UNSET(state,(CLK | DT)) && IS_SET(state, OUTER)) 

 
static line_t *gpi[MAXGPIO] = { 0 };
static void (*user_callback)();

static unsigned int offsets[MAXGPIO] = { 0 };
static int num_lines = 0;
static int shutdown = 0;

static char consumer[MAXNAME];
static char device[MAXNAME];

static unsigned long long usec_stamp(struct timespec t)
{
	return (unsigned long long)((t.tv_sec * 1000000ULL) + (t.tv_nsec / 1000ULL));
}

static char* uint_pp(unsigned int bitfield, int nbits) {
	char* output = calloc(sizeof(char), nbits);
	for (int i=0; i<nbits; i++) {
		int shift = nbits - i - 1;
		output[i] = (char)(((bitfield & (1U << shift)) >> shift ) + '0');
	}
	return output;	
}

static int handle_event(int event, unsigned int line, const struct timespec *timestamp,
	     void *data)
{
	unsigned long long now;
	int value;
	unsigned int* state;

	if (shutdown)
		return GPIOD_CTXLESS_EVENT_CB_RET_STOP;

	now = usec_stamp(*timestamp);
	DBG("GPIOD handler at time %lld", now);
	value = (event == GPIOD_CTXLESS_EVENT_CB_RISING_EDGE) ? 1 : 0;
	if ((now - gpi[line]->ts_last) > gpi[line]->ts_delta) {
		// we're not bouncing:
		gpi[line]->ts_last = now;
		switch (gpi[line]->type) {
		case GPI_ROTARY:
			// lazy hack:
			// store current value in aux field,
			// because an aux can't have an aux.
			state = &(gpi[gpi[line]->aux]->aux);
			UPDATE(*state, CLK, ~0 * value);
			break;
		case GPI_AUX:
			state = &(gpi[line]->aux);
			UPDATE(*state, DT, ~0 * value);
			break;
		case GPI_SWITCH:
			user_callback(line, 1 - value); // look for falling edge
			return GPIOD_CTXLESS_EVENT_CB_RET_OK; // skip state machine
			break;
		default:
			ERR("No handler for type %d. THIS SHOULD NEVER HAPPEN.",
			    gpi[line]->type);
			return GPIOD_CTXLESS_EVENT_CB_RET_ERR;
			break;
		}
		DBG("state before: %s", uint_pp(*state, 4));
		// which direction?
		if (IS_SET(*state, OUTER)) {
			if (IS_UNSET(*state, CLK) && IS_SET(*state, DT)) 
				SET(*state, CLOCKWISE);
			else if (IS_SET(*state, CLK) && IS_UNSET(*state, DT))
				UNSET(*state, CLOCKWISE);
		} else {
			if (IS_SET(*state, CLK) && IS_UNSET(*state, DT)) 
                                SET(*state, CLOCKWISE);
                        else if (IS_UNSET(*state, CLK) && IS_SET(*state, DT))
                                UNSET(*state, CLOCKWISE);
		}
		if (FIRE_INNER(*state)) {
			user_callback(line, -1 + 2 * IS_SET(*state, CLOCKWISE));	
			SET(*state, OUTER);
		} else if (FIRE_OUTER(*state)) {
			user_callback(line, -1 + 2 * IS_SET(*state, CLOCKWISE));
			UNSET(*state, OUTER);
		}
		DBG("state after: %s", uint_pp(*state, 4));
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
	gpi[line]->aux = aux;
	gpi[line]->ts_last = NEVER;
	gpi[line]->ts_delta = GPI_DEBOUNCE_ROTARY;
	gpi[aux]->type = GPI_AUX;
	gpi[aux]->aux = 0;
	gpi[aux]->ts_last = NEVER;
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
		if (gpi[line] != NULL) {
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
