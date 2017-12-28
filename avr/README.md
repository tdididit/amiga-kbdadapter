# AKAB: Amiga Keyboard Adapter Board

## Description
AKAB is an implementation of an adapter to connect PS/2 keyboards to Amiga
computers.  
It's based on an AVR **ATMega328P** microcontroller which does all the required work
to translate between PS/2 and Amiga keycodes.

## Compilation instructions
With the appropriate packages installed in your distribution of choice
(_tested on GNU/Linux only_), compiling should be a simple matter of
running `make` in the project root directory. The output files will be
placed in the `out` directory.

The list of packages currently required in __Debian testing__ to complete
the build:
`
avr-libc
binutils-avr
gcc-avr
`

Also, to load the firmwre in the micro `avrdude` is really great.  
In fact you can run `make program` to try to program the micro via _AVRDragon_
and _ISP_. 
Change the _Makefile_ to adapt for other programmers.

## Additional notes
You can find AVR fuses inside the Makefile, but just for reference: 
`hfuse = 0xD9, lfuse = 0xE2`

Right now, the communication is unidirectional, that is PS/2 to Amiga only.
This means that **the Amiga is not yet able to blink the leds on the PS/2
keyboard**.

Schematics in _kicad_ and _pdf_ format are available. Check in `schematics`

## Disclaimer
I take absolutely no responsibility for anything that may happen if you use
code, schematics or information from this project.
It may burn your keyboard port, make the computer explode, have you lose 
your happiness or cause other nasty events, so beware.

## Licence
This project is released under the GPLv3 licence.  
Please, drop me a line at _hkzlabnet+AT+gmail+DOT+com_ (replace parts
between ++) if you use this project for something cool!


