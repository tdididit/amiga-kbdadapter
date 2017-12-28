#include <stdint.h>
#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "ps2_keyb.h"
#include "ps2_proto.h"

#include "amiga_keyb.h"

#include "ps2_converter.h"

#include "main.h"


int main(void) {
	uint8_t keyb_commands[2];

	// Set the pull-up resistor to all unused I/O ...
	DDRB &= 0x03;
	PORTB |= 0xFC;
	
	DDRC &= 0xC0;
	PORTC |= 0x3F;

	DDRD &= 0x0C;
	PORTD |= 0xF3;

	_delay_ms(50);


	// Initialization of PS/2 and Amiga interface
	amikbd_setup(&PORTB, &DDRB, 0, &PORTD, &DDRD, 0);

	ps2keyb_init(&PORTB, &DDRB, &PINB, 1);
	ps2keyb_setCallback(ps2k_callback);

	// Force the keyboard reset
	keyb_commands[0] = PS2_HTD_RESET;
	ps2keyb_sendCommand(keyb_commands, 1); // This also enables interrupts

	amikbd_init();

	while(1);

    return 0;
}


