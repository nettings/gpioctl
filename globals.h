#ifndef GLOBALS_H
#define GLOBALS_H

#define JACK_CLIENT_NAME "rot2midi"
#define JACK_PORT_NAME "midi_out"
#define GPIOD_DEVICE "pinctrl-bcm2835"
#define MAXGPIO 32
#define MIDI_MAX 0x7f
#define MIDI_CC 0xb
#define MSG_SIZE 3
#define MAXNAME 64

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
        int midi_ch;
        int midi_cc;
} jack_rotary_t;

typedef struct {
        int clk;
        int dt;
        int step;
        char mixer_scontrol[MAXNAME];
} amixer_rotary_t;

typedef struct {
        int sw;
        int midi_ch;
        int midi_cc;
        int toggled;
} jack_switch_t;

typedef struct {
        int sw;
        char mixer_scontrol[MAXNAME];
} amixer_mute_t;


#endif

