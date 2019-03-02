# gpioctl

gpioctl has been written to provide hardware volume control for an audio
engine running on the Raspberry Pi, via a rotary encoder connected to the
GPIOs. It can directly interact with the ALSA mixer, or you can create JACK
MIDI messages to remote-control any JACK MIDI capable client.
Since it uses a generic GPIO interface via libgpiod, it might also be useful on
other hardware platforms. I cannot test this personally, but your feedback
is welcome. 

gpioctl is written in C because it turned out that I couldn't get
Python-based solutions to perform well enough to work with JACK at low
latencies. If you don't need JACK, you might find Python friendlier to work
with. A good example is this one:
https://gist.github.com/savetheclocktower/9b5f67c20f6c04e65ed88f2e594d43c1
 
## Usage

```
$ ./build/gpioctl -h

gpioctl handles switches and rotary encoders connected to GPIOs, using the
portable libgpiod kernel interface, to send JACK MIDI CC messages via 
gpioctl:midi_out, directly interact with an ALSA mixer control, or print formatted
values to stdout.
We assume GPI pins have a pull-up, so the return should be connected to ground.
-h|--help      This help.
-v|--verbose   Print current controller values.

The following options may be specified multiple times. All parameters must be
separated by commas, no spaces. Parameters in brackets are optional.

-r|--rotary clk,dt,type,...
               Set up a rotary encoder.
               clk:     the GPI number of the first encoder contact (0-63)
               dt:      the GPI number of the second encoder contact (0-63)
               Depending on 'type', the remaining parameters are:

      ...,jack,cc,[ch[,min[,max[,step[,default]]]]]
               cc:      MIDI continous controller number (0-119)
               ch:      MIDI channel (1-16), default 1
               min:     minimum controller value (0-127), default 0
               max:     maximum controller value (0-127), default 127
               step:    the step size per 'click'(1-127), default 1
               default: the initial value, default is 'min'

      ...,alsa,control[,step]
               control: the name of a simple controller in ALSA mixer
               step: the step size in dB per click, default 3

     ...,stdout,format[,min[,max[,step[,default]]]]].
               format:  a string that can contain the special tokens '%gpi%'
                        (the pin number) and '%val%' (the value)
               min:     minimum value (-2147483648 - 2147483647), default 0
               max:     maximum value (-2147483648 - 2147483647), default 100
               step:    the step size per click, default 1
               default:	the initial value, default is 'min'

-s|--switch sw,type...
               Set up a switch.
               sw:      the GPI pin number of the switch contact (0-63)
               Depending on 'type', the remaining parameters are:

      ...,jack,cc,[ch[,toggle[,min[,max[,default]]]]]
               cc:      MIDI continous controller number (0-120)
               ch:      MIDI channel (1-16), default 1
               toggle:  can be 0 (momentary on) or 1 (toggled on/off)
               min:     controller value when open (0-127), default 0
               max:     controller value when closed (0-127), default 127
               default: the initial value, default is 'min'

      ...,alsa,control
               control: the name of a simple controller in ALSA mixer
                        (switch will operate the MUTE function)
      ...,stdout,format[,toggle[,min[,max[,default]]]]
               format:  a string that can contain the special tokens '%gpi%'
                        (the pin number) and '%val%' (the value)
               toggle:  can be 0 (momentary on) or 1 (toggled on/off)
               min:     minimum value (-2147483648 - 2147483647), default 0
               max:     maximum value (-2147483648 - 2147483647), default 1
               default:	the start value, default is 'min'

Pin numbers above are hardware GPIO numbers. They do not usually correspond
to physical pin numbers. For the RPi, check https://pinout.xyz/# and look
for the Broadcom ('BCM') numbers.
libgpiod does not know how to control the pull-up/pull-down resistors of your
GPIO pins. Use a hardware-specific external tool to enable them, or add
physical pull-ups.

gpioctl is meant to run as a daemon. Use CTRL-C or send a SIGTERM to exit.

```

## Connecting rotary encoders

For testing, I'm using an ALPS incremental rotary encoder [(model no. 
EC11E15244C0)](https://nl.farnell.com/webapp/wcs/stores/servlet/ProductDisplay?urlRequestType=Base&catalogId=10001&langId=31&storeId=10168&partNumber=1520806)
with push-down switch. This one has three pins on one side for the rotary
(clk, ret, dt), and two on the other for the switch (sw, ret).

A rotary works with two switches which open and close slightly out-of-phase.
If you get an edge interrupt on [clk] and both switches are the same (either
open or closed), you know you just went one click clockwise.
If the switches are in opposite states, you went one click
counter-clockwise.

Here's a Raspberry Pi 3B+ with a HifiBerry AMP2 the ALPS rotary
encoder/switch connected to the GPIOs. I'm using GPIOs 17 (white), 27 (grey), 
6 (purple) in this example, the ground is black. Check out the pins at
https://pinout.xyz/#.

![Figure 1](doc/RaspberryPi3B+_HifiBerryAMP2_ALPSRotaryEnc.jpg "A Raspberry Pi 3B+ and
HifiBerry AMP2 with an ALPS rotary encoder as Master Volume")

The wiring side: the rotary connections are visible on the left, with the
return pin in the middle tied to the return of the switch and grounded.
The solder blobs at the top and bottom don't do anything, they just secure
the encoder case in place.

![Figure 2](doc/Wiring.jpg "The rotary is on the left, with the ground pin
in the middle tied to the return of the switch on the right.")

In case you were wondering: you can use as many encoders as you have GPIs
(tested to up to four):

![Figure 3](doc/Board_with_four_Encoders.jpg "A board with four ALPS
pushbutton encoders wired to a cannibalized ribbon cable.")

## Enabling the pull-up resistors

libgpiod will set the pin direction to "input" automatically, but it is not
currently able to set other pin features. So you will have to use a 
hardware-specific tool to enable pull-ups, or connect your controller board to
an appropriate voltage source and add physical pull-ups.

On the Raspberry Pi, you can use the `gpio` command that comes with
wiringpi:

```
$ sudo apt-get install wiringpi         # if you don't have it yet
$ gpio -g mode 17 up
$ gpio -g mode 27 up
$ gpio -g mode 6 up
```

AFAIK, it is not possible to read out the state of the pull-ups on the Pi,
but you can verify the correct setting by running `gpio readall` and then
run it again after moving the rotary by one click or while holding down the
switch. The value ("V") column should change.

If your pin requirements do not change at runtime, you can also  preconfigure
your pin states via
`[/boot/config.txt](https://www.raspberrypi.org/forums/viewtopic.php?f=117&t=208748)`.

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

Once you've found the appropriate control name, plug it into the following 
command instead of "Digital", which is the default for HifiBerry AMP2 users.
Now you can run
```
$ gpioctl -v -r 17,27,alsa,Digital -s 6,alsa
```
You can also run 
```
$ watch -n 0.5 amixer sget [YOURCONTROL]
```
or
```
$ alsamixer
```
in another terminal and watch the mixer update live.

## Sending JACK MIDI commands

Start a JACK server. Then open another terminal and run
```
$ jack_midi_dump
```
which will dump all incoming JACK MIDI messages to the screen.

In a third terminal, run
```
$ gpioctl -v -r 17,27,jack,1,15 -s 6,jack,1,16,1
```
If all goes well, you should see two new jack clients, which you connect: 
```
$ jack_lsp
system:playback_1
system:playback_2
midi-monitor:input
gpioctl:midi_out
$ jack_connect gpioctl:midi_out midi-monitor:input
```
Now use the controls and watch the JACK MIDI events coming in.
Of course the point is to use another JACK client that does useful things
with those controller inputs. Ardour or mod-host are examples.

## Using the stdout frontend

The most simple way of using gpioctl is to have it spit out controller
values to standard output. The formatting option is currently not
implemented and will be ignored.
```
$ gpioctl -r 17,27,stdout,FOOBAR -s 6,stdout,FOOBAR,1
```


## Building gpioctl

In addition to the usual system header files and libraries, gpioctl requires
`libgpiod-dev`. If you want to use JACK MIDI, you need `libjack-jackd2-dev` 
or `libjack-dev` (untested). If you want to access the ALSA mixer, you need
`libasound2-dev`.
I recommend installing `wiringpi` on the Pi or another hardware-specific GPIO 
controller tool for your platform. (Package names are for Raspbian, they may
differ on your system.)

The build system is waf. My understanding of it is very limited. For
now, from the root your working copy do
```
$ CFLAGS="-DDEBUG -g" ./waf configure  # for very verbose output, or
$ ./waf configure                      # for production code, or
$ ./waf configure --prefix=/foo        # if you don't like /usr/local/, and
$ ./waf                                # to build
```
You can run it without installing from ./build/gpioctl, or install it with
```
$ sudo ./waf install                   # installs to prefix set before, or
$ sudo ./waf install --destdir=/bar    # for packagers
```
