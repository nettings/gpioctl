#ifndef GLOBALS_H
#define GLOBALS_H

#define JACK_CLIENT_NAME "gpioctl"
#define JACK_PORT_NAME "midi_out"
#define GPIOD_DEVICE "pinctrl-bcm2835"
#define MAXGPIO 32
#define MIDI_MAX 0x7f
#define MIDI_CC 0xb
#define MSG_SIZE 3
#define MAXNAME 64

#include <stdio.h>

#ifdef DEBUG
#define DBG(fmt, args...) fprintf(stdout, "%s:%d %s(): " fmt "\n", __FILE__, __LINE__, __func__,  ## args)
#define ERR(fmt, args...) fprintf(stderr, "%s:%d %s(): \x1b[01;31m" fmt "\x1b[0m\n", __FILE__, __LINE__, __func__, ## args)
#else
#define DBG(fmt, args...) 
#define ERR(fmt, args...) fprintf(stderr, "\x1b[31m"fmt"\x1b[0m\n", ## args)
#endif


int verbose;

enum ctl_types {
        UNUSED,
        JACKROT,
        JACKSW,
        ALSAROT,
        ALSASW
};

typedef struct {
        enum ctl_types type;
        void *data;
} controller_t;

controller_t controllers[MAXGPIO];

typedef struct {
        int clk;
        int dt;
        int step;
        unsigned char midi_ch;
        unsigned char midi_cc;
        unsigned char counter;
} jack_rotary_t;

typedef struct {
        int clk;
        int dt;
        int step;
        char mixer_scontrol[MAXNAME];
} amixer_rotary_t;

typedef struct {
        int sw;
        unsigned char midi_ch;
        unsigned char midi_cc;
        int toggled;
} jack_switch_t;

typedef struct {
        int sw;
        char mixer_scontrol[MAXNAME];
} amixer_mute_t;


#endif

