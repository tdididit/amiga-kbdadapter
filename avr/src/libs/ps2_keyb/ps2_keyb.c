#include "ps2_keyb.h"
#include "ps2_proto.h"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <stdio.h>

// See the following link for details on PS/2 protocol
// http://www.computer-engineering.org/ps2protocol/
// See the following link for details on the keyboard PS/2 commands
// http://www.computer-engineering.org/ps2keyboard/

// Data port
static volatile uint8_t *dPort, *dPin, *dDir;
static volatile uint8_t dPNum; // Data port pin (leg) number

// Clock port
static volatile uint8_t *cPort, *cPin, *cDir;
static volatile uint8_t cPNum;

#define KB_CLOCK_FALL 0
#define KB_CLOCK_RISE 1
#define PS2_START_BITCOUNT 11 // 12 bits is only for host-to-device communication

#define KEY_BUF_SIZE 8

static volatile uint8_t clock_edge;
static volatile uint8_t kb_bitCount;

static volatile uint8_t keyBuffer[KEY_BUF_SIZE];
static volatile uint8_t *kb_inPtr, *kb_outPtr, *kb_endPtr;

#define KB_START_BIT(a) ((a >> 0) & 0x01)
#define KB_PARITY_BIT(a) ((a >> 1) & 0x01)
#define KB_STOP_BIT(a) ((a >> 2) & 0x01)

#define KB_SET_START_BIT(a, b) (a |= (b << 0))
#define KB_SET_PARITY_BIT(a, b) (a |= (b << 1))
#define KB_SET_STOP_BIT(a, b) (a |= (b << 2))

void ps2_dumb_print(uint8_t *code, uint8_t count);

void static (*keypress_callback)(uint8_t *code, uint8_t count) = ps2_dumb_print;
static volatile uint8_t kb_data, kb_flag;

int kb_parity_check(uint8_t kb_flag_i, uint8_t kb_data_i);
void kb_pushScancode(uint8_t code);

void ps2_dumb_print(uint8_t *code, uint8_t count) {
	//printf("%.2X %.2X %.2X\n", code[0], code[1], code[2]);
}

int kb_parity_check(uint8_t kb_flag_i, uint8_t kb_data_i) {
	uint8_t result = 1;
	uint8_t counter = 8;

	while (counter--) {
		result = kb_data_i & 0x1 ? !result : result;
		kb_data_i >>= 1;
	}

	return (result == KB_PARITY_BIT(kb_flag_i));
}

// See http://avrprogrammers.com/example_avr_keyboard.php
// http://elecrom.wordpress.com/2008/02/12/avr-tutorial-2-avr-input-output/
void ps2keyb_init(volatile uint8_t *dataPort, volatile uint8_t *dataDir, volatile uint8_t *dataPin, uint8_t pNum) {
	dPort = dataPort;
	dPin = dataPin;
	dDir = dataDir;
	dPNum = pNum;

	cPort = &PORTD;
	cPin = &PIND;
	cDir = &DDRD;
#if defined (__AVR_ATmega128__)
	cPNum = 0; // PD0
#elif defined (__AVR_ATtiny4313__) || defined (__AVR_ATmega328P__) || defined (__AVR_ATmega8A__)
	cPNum = 2; // PD2
#endif

	// Prepare data port
	*dDir &= ~(1 << dPNum); // KB Data line set as input
	*dPort |= (1 << dPNum); // Pull-up resistor on data line

	// Prepare clock port
	*cDir &= ~(1 << cPNum); // KB Clock line set as input
	*cPort |= (1 << cPNum); // Pull-up resistor on clock line

	// See http://www.avr-tutorials.com/interrupts/The-AVR-8-Bits-Microcontrollers-External-Interrupts
	// And http://www.atmel.com/images/doc2543.pdf

#if defined (__AVR_ATmega128__) || defined (__AVR_ATmega328P__)
	EICRA &= ~((1 << ISC00) | (1 << ISC01)); 
	EICRA |= (1 << ISC01);  // Trigger interrupt at FALLING EDGE (INT0)
	EIMSK |= (1 << INT0);
#elif defined (__AVR_ATtiny4313__) || defined (__AVR_ATmega8A__)
	MCUCR &= ~((1 << ISC00) | (1 << ISC01)); 
	MCUCR |= (1 << ISC01);  // Trigger interrupt at FALLING EDGE (INT0)
#endif

	// I suspect this to be totally useless...
	//PCMSK |= (1<<PIND2);	// Enable pin change on INT0 (why is this required?)

	clock_edge = KB_CLOCK_FALL;
	kb_bitCount = PS2_START_BITCOUNT;

	kb_data = 0;
	kb_flag = 0;

	// Prepare the ring buffer...
	kb_inPtr = kb_outPtr = keyBuffer;
	kb_endPtr = kb_inPtr + KEY_BUF_SIZE;

	// Enable INT0
#if defined (__AVR_ATmega128__) || defined (__AVR_ATmega328P__)
	EIMSK |= (1 << INT0);
#elif defined (__AVR_ATtiny4313__)
	GIMSK |= (1 << INT0);
#elif defined (__AVR_ATmega8A__)
	GICR  |= (1 << INT0); // Enable INT1
#endif
}

void kb_pushScancode(uint8_t code) {
	static uint8_t code_array[9];
	static uint8_t cur = 0;

	code_array[cur] = code;

	switch (code) {
		case PS2_SCANCODE_RELEASE: // Key released, expect at least another code!
		case PS2_SCANCODE_EXTENDED: // Extended scancode, one or two more!
		case PS2_SCANCODE_PAUSE: // Pause ...
			cur++;
			break;
		default:
			// Manage pause and printscreen
			if (((code_array[0] == PS2_SCANCODE_EXTENDED) && (code_array[1] == 0x12) && (code_array[3] != 0x7C)) ||
				((code_array[0] == PS2_SCANCODE_EXTENDED) && (code_array[1] == 0xF0) && (code_array[2] == 0x7C) && (code_array[5] != 0x12)) ||
				(code_array[0] == PS2_SCANCODE_PAUSE && cur < 7)) {
				cur++;
				break;
			}

			(*keypress_callback)(code_array, cur);

			cur = 0;
			code_array[0] = code_array[1] = code_array[2] = 0;
			code_array[3] = code_array[4] = code_array[5] = 0;
			code_array[6] = code_array[7] = code_array[8] = 0;

			break;
	}
}

void ps2keyb_setCallback(void (*callback)(uint8_t *code, uint8_t count)) {
	keypress_callback = callback;
}

// See http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&t=134386
void ps2keyb_sendCommand(uint8_t *command, uint8_t length) {
	uint8_t cur_data = 0;
	uint8_t parity_check;

	// Send host-to-device command...

	cli(); // Disable all interrupts in preparation to command sending

	// Iterate over all the data bytes we have to send
	for (uint8_t idx = 0; idx < length; idx++) {
		// Bring the clock line LOW for at least 100 microseconds
		*cDir |= (1 << cPNum); // KB Clock line set as output
		*dDir |= (1 << dPNum); // KB Data line set as output

		*cPort &= ~(1 << cPNum); // bring clock line LOW
		_delay_us(100);

		// Apply a request-to-send by bringing data line low
		*dPort &= ~(1 << dPNum); // Bring data line LOW

		// Release the clock port (set it to floating and give control back)
		*cDir &= ~(1 << cPNum); // KB Clock line set as input
		*cPort |= (1 << cPNum); // Pull-up resistor on clock line

		// And wait for the device to bring clock line LOW
		while (*cPin & (1 << cPNum));

		// Now begin send the data bits...
		cur_data = command[idx];
		parity_check = 1;
		for (uint8_t bit_idx = 0; bit_idx < 8; bit_idx++) {
			if (cur_data & 0x01) {  // Set the line to floating with pullup
				*dDir &= ~(1 << dPNum); // KB Data line set as input
				*dPort |= (1 << dPNum); // Pull-up resistor on data line

				if (!parity_check) parity_check = 1;
				else parity_check = 0;
			} else {
				*dDir |= (1 << dPNum); // KB Data line set as output
				*dPort &= ~(1 << dPNum); // Force it low
			}

			cur_data >>= 1;
			
			// Wait for the device to bring the clock high and then low
			while (!(*cPin & (1 << cPNum)));
			while (*cPin & (1 << cPNum));
		}

		// Send the parity bit
		if (parity_check) {
			*dDir &= ~(1 << dPNum); // Force the line as floating again
			*dPort |= (1 << dPNum); // Pull-up resistor on data line
		} else {
			*dDir |= (1 << dPNum); // KB Data line set as output
			*dPort &= ~(1 << dPNum); // And force it low
		}
		// Wait for the device to bring the clock high and then low
		while (!(*cPin & (1 << cPNum)));
		while (*cPin & (1 << cPNum));

		// Parity sent, now set the data high (floating)
		*dDir &= ~(1 << dPNum); // KB Data line set as input
		*dPort |= (1 << dPNum); // Pull-up resistor on data line
		
		// Wait for the device to bring the clock high and then low
		while (!(*cPin & (1 << cPNum)));
		while (*cPin & (1 << cPNum));
		
		// Set the data line low
		*dDir |= (1 << dPNum); // KB Data line set as output
		*dPort &= ~(1 << dPNum); // Pull the line low
		
		*dDir &= ~(1 << dPNum); // KB Data line set as input
		
		// Wait for clock line to get high
		while (!(*cPin & (1 << cPNum)));
		while (*cPin & (1 << cPNum)); // Then low
		while (!(*cPin & (1 << cPNum))); // Then high

		// Wait for the data line to get high
		while (!(*dPin & (1 << dPNum)));

		_delay_ms(15); // Wait for the device to be ready again
	}

	// Prepare data port
	*dDir &= ~(1 << dPNum); // KB Data line set as input
	*dPort |= (1 << dPNum); // Pull-up resistor on data line

	// Prepare clock port
	*cDir &= ~(1 << cPNum); // KB Clock line set as input
	*cPort |= (1 << cPNum); // Pull-up resistor on clock line

	sei();
}

ISR(INT0_vect) { // Manage INT0
	uint8_t kBit = 0;

	if (clock_edge == KB_CLOCK_FALL) { // Falling edge
		kBit = (*dPin & (1 << dPNum)) ? 1 : 0;

		// bit 0 is start bit, bit 9,10 are parity and stop bits
		// What is left are the data bits!
		if (kb_bitCount < 11 && kb_bitCount > 2) { 
			kb_data >>= 1; // Shift the data

			if (kBit) kb_data |= 0x80; // Add a bit if the read data is one
		} else if (kb_bitCount == 11) { // start bit, must always be 0!
			KB_SET_START_BIT(kb_flag, kBit);
		} else if (kb_bitCount == 2) { // Parity bit: 1 if there is an even number of 1s in the data bits
			KB_SET_PARITY_BIT(kb_flag, kBit);			
		} else if (kb_bitCount == 1) { // Stop bit, must always be 1!
			KB_SET_STOP_BIT(kb_flag, kBit);	
		}
		clock_edge = KB_CLOCK_RISE;			// Ready for rising edge.

#if defined (__AVR_ATmega128__) || defined (__AVR_ATmega328P__)
		EICRA |= ((1 << ISC00) | (1 << ISC01)); // Setup INT0 for rising edge.
#elif defined (__AVR_ATtiny4313__) || defined (__AVR_ATmega8A__)
		MCUCR |= ((1 << ISC00) | (1 << ISC01)); // Setup INT0 for rising edge.
#endif
	} else { // Rising edge
		if(!(--kb_bitCount)) {
			if (!KB_START_BIT(kb_flag) && KB_STOP_BIT(kb_flag) && kb_parity_check(kb_flag, kb_data)) {
				kb_pushScancode(kb_data);
			} // Else... there was a problem somewhere, probably timing

			kb_data = 0;
			kb_flag = 0;

			kb_bitCount = PS2_START_BITCOUNT; // Start over.
		}
		clock_edge = KB_CLOCK_FALL;		// Setup routine the next falling edge.

#if defined (__AVR_ATmega128__) || defined (__AVR_ATmega328P__)
		EICRA &= ~((1 << ISC00) | (1 << ISC01)); 
		EICRA |= (1 << ISC01);  // Trigger interrupt at FALLING EDGE (INT0)
#elif defined (__AVR_ATtiny4313__) || defined (__AVR_ATmega8A__)
		MCUCR &= ~((1 << ISC00) | (1 << ISC01)); 
		MCUCR |= (1 << ISC01);  // Trigger interrupt at FALLING EDGE (INT0)
#endif
	}
}



