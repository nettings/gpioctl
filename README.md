# gpioctl

gpioctl has been written to provide hardware volume control for an audio
engine running on the Raspberry Pi, via a rotary encoder connected to the
GPIOs. It can directly interact with the ALSA mixer, or you can create JACK
MIDI messages to remote-control any JACK MIDI capable client.
Since it uses a generic GPIO interface via libgpiod, it might also be useful on
other hardware platforms. I cannot test this personally, but your feedback
is welcome. 

## Usage

$ ./build/gpioctl -h
'''
gpioctl handles switches and rotary encoders connected to GPIOs, using
the portable libgpiod kernel interface, to create JACK MIDI CC messages
at gpioctl:midi_out or directly interact with an ALSA mixer control.
All GPI pins are pulled up, so the return should be connected to ground.

-h|--help      This help.
-v|--verbose   Print current controller values.
-J|--jack-rotary clk,dt,ch,cc,step
               Set up a rotary encoder on pins [clk] and [dt], and create MIDI
               messages on channel [ch] for CC no. [cc] with step size [step].
-A|--amixer-rotary clk,dt,name,step
               Set up a rotary encoder on pins [clk] and [dt], and control
               ALSA mixer element [name] with step size [step].
               Use 'amixer scontrols' to get a list.
-j|--jack-switch sw,ch,cc,toggled
               Set up a switch on pin [sw], and create MIDI messages on channel
               [ch] for CC no. [cc]. If [toggled] is 0, the switch will send
               value '127' when pressed, and '0' when released. With [toggled]
               at 1, one press will latch it to '127', and the next one will
               release it to '0'.
-a|--amixer-mute sw,mixer_scontrol
               Set up a switch on pin [sw], and toggle the MUTE status on ALSA
               mixer element [name].

The options [JAja] may be specified multiple times.
'''

## Connecting rotary encoders

For testing, I'm using an ALPS incremental rotary encoder [(model no. 
EC11E15244C0)](https://nl.farnell.com/webapp/wcs/stores/servlet/ProductDisplay?urlRequestType=Base&catalogId=10001&langId=31&storeId=10168&partNumber=1520806)
with push-down switch. This one has three pins on one side for the rotary
(clk, ret, dt), and two on the other for the switch (sw, ret).

gpioctl initializes all GPIS with pull-up resistors, so that both [ret] pins
can be connected to ground.

A rotary works with two switches which open and close slightly out-of-phase.
If you get an edge interrupt on [clk] and both switches are the same (either
open or closed), you know you just went one click clockwise.
If the switches are in opposite states, you went one click
counter-clockwise.