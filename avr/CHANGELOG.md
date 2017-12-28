## AKAB Changelog

### 2017-03-23 **(0.3-alpha)*
* Added support for ATmega8A by Peter Zelezny

### 2016-05-16 **(0.2-alpha)**
* Do not set HIGH data/clock lines on Amiga side ever: let the pullup resistor do their job.
  * Previous versions explicitly set lines high, this is wrong and dangerous.

### 2016-05-10 **(0.1)**
* Initial import from AVR-Examples
