#include "gpiod_process.h"
#include <gpiod.h>
#include <stdlib.h>
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

typedef struct {
	int aux;
	unsigned long ts_last;
	int ts_delta;
	int (*cb)();
} line_t;

enum line_types {
	AUX = -1,		// used as secondary port for other line, no separate interrupt handler
	FREE = 0,
	ROTARY = 1,
	SWITCH = 2
};

typedef struct {
	enum line_types type;
	line_t *data;
} line_handler_t;

static line_handler_t line_handler[MAXGPIO] = { 0 };
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
	line_t *d = line_handler[line].data;
	now = msec_stamp(*timestamp);
	if ((now - d->ts_last) > d->ts_delta) {
		// we're not bouncing:
		switch (line_handler[line].type) {
		case ROTARY:
			clk =
			    (event ==
			     GPIOD_CTXLESS_EVENT_CB_RISING_EDGE) ? 1 : 0;
			dt = gpiod_ctxless_get_value(device, d->aux,
						     ACTIVE_HIGH, consumer);
			if (clk != dt) {
				d->cb(line, 1);
			} else {
				d->cb(line, -1);
			}
			break;
		case SWITCH:
			sw =
		           (event == GPIOD_CTXLESS_EVENT_CB_FALLING_EDGE) ? 1 : 0;
		        d->cb(line, sw);
			break;
		default:
			ERR("No handler for type %d. THIS SHOULD NEVER HAPPEN.", line_handler[line].type);
			return GPIOD_CTXLESS_EVENT_CB_RET_ERR;
			break;
		}
		d->ts_last = now;
	}
	return GPIOD_CTXLESS_EVENT_CB_RET_OK;
}

int null_callback(int event, unsigned int line, const struct timespec *timeout,
		  void *data)
{
	ERR("This should never be called.");
	return GPIOD_CTXLESS_EVENT_CB_RET_ERR;
}

void setup_gpiod_rotary(int clk, int dt, void (*user_callback))
{
	DBG("int clk=%d, int dt=%d, void (*user_callback)=%p", clk, dt, user_callback);
	if (line_handler[clk].type != FREE) {
		ERR("Line %d is already in use: %d.", clk, line_handler[clk].type);
		return;
	}
	if (line_handler[dt].type != FREE) {
		ERR("Aux %d is already in use: %d.", dt, line_handler[dt].type);
		return;
	}
	if (clk == dt) {
		ERR("Line and Aux line cannot both be %d.", clk);
		return;
	}
	line_t *d = malloc(sizeof(line_t));
	if (d == NULL) {
		ERR("malloc() failed.");
		return;
	}
	line_handler[clk].type = ROTARY;
	line_handler[dt].type = AUX;
	line_handler[dt].data = NULL;
	line_handler[clk].data = d;
	d->aux = dt;
	d->ts_last = NEVER;
	d->ts_delta = 20;
	d->cb = user_callback;
}

void setup_gpiod_switch(int sw, void (*user_callback))
{
	if (line_handler[sw].type != FREE) {
		ERR("Line %d is already in use: %d.", sw, line_handler[sw].type);
		return;
	}
	line_t *d = malloc(sizeof(line_t));
	if (d == NULL) {
		ERR("malloc() failed.");
		return;
	}
	line_handler[sw].type = SWITCH;
	line_handler[sw].data = d;
	
	d->aux = NOAUX;
	d->ts_last = NEVER;
	d->ts_delta = 100;
	d->cb = user_callback;
}

void shutdown_gpiod()
{
	gpiod_ctxless_event_loop_multiple(device, NULL, 0, ACTIVE_HIGH,
					  consumer, FOREVER, NULL,
					  &null_callback, NULL);
	for (int i = 0; i < MAXGPIO; i++) {
		line_handler[i].type == FREE;
		if (line_handler[i].data != NULL) {
			free(line_handler[i].data);
			line_handler[i].data == NULL;
		}
	}
}

void setup_gpiod_handler(char *dev, char *cons)
{
	unsigned int offsets[MAXGPIO];
	int num_lines = 0;
	int err = 0;
	for (int i = 0; i < MAXGPIO; i++) {
		if (line_handler[i].type > FREE) {
			DBG("Added Pin %d in position %d.", i, num_lines);
			offsets[num_lines++] = i;
		}
	}
	strncpy (consumer, cons, MAXNAME);
	strncpy (device, dev, MAXNAME);
	err = gpiod_ctxless_event_loop_multiple(device, offsets, num_lines,
					  ACTIVE_HIGH, consumer, FOREVER, NULL,
					  callback, NULL);
	ERR("gpiod_ctxless_event_loop_multple: err = %d, errno = %d.", err, errno);
}

