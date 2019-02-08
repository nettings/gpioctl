# gpioctl

gpioctl has been written to provide hardware volume control for an audio
engine running on the Raspberry Pi, via a rotary encoder connected to the
GPIOs. It can directly interact with the ALSA mixer, or you can create JACK
MIDI messages to remote-control any JACK MIDI capable client.
Since it uses a generic GPIO interface via libgpiod, it might also be useful on
other hardware platforms. I cannot test this personally, but your feedback
is welcome. 

## Usage

```
$ ./build/gpioctl -h

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
-a|--amixer-mute sw,name
               Set up a switch on pin [sw], and toggle the MUTE status on ALSA
               mixer element [name].

The options [JAja] may be specified multiple times.

Pin numbers below are hardware GPIO numbers. They do not usually correspond
to physical pin numbers. For the RPi, check https://pinout.xyz/# and look
for the Broadcom ('BCM') numbers.

```

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

Here's a Raspberry Pi 3B+ with a HifiBerry AMP2 and an ALPS rotary encoder with
switch connected to the GPIOs. I'm using GPIOs 17 (white), 27 (grey), 6
(purple) in this example, the ground is black.

![Figure 1](doc/RaspberryPi3B+_HifiBerryAMP2_ALPSRotaryEnc.jpg "A Raspberry Pi 3B+ and
HifiBerry AMP2 with an ALPS rotary encoder as Master Volume")

 The command I'm using is
```
$ gpioctl -v -A 17,27,Digital,3 -a 6,Digital
```
It will tie the rotary to the volume, and operate the MUTE switch with the
pushdown function.

The wiring side: the rotary connections are visible on the left, with the
return pin in the middle tied to the return of the switch and grounded.
The solder blobs at the top and bottom don't do anything, they just secure
the encoder case in place.

![Figure 2](doc/Wiring.jpg "The rotary is on the left, with the ground pin
in the middle tied to the return of the switch on the right.")


## Controlling the ALSA mixer

In order to find the right mixer control, play some music and look at 
```
$ amixer scontrols
``` 
Then try to manipulate a control with 
```
$ amixer sset [YOURCONTROL] 30%
```
and see if the playback volume changes.

Once you've found the appropriate control name, plug it into the command
above (for HifiBerry AMP2 users, it's "Digital").
If you want to see what's going on, use the -v switch.
You can also run 
```
$ watch -n 0.5 amixer sget [YOURCONTROL]
```
in another terminal and watch the mixer update live.
