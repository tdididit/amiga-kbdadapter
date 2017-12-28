#include "amiga_keyb.h"

#include <stdio.h>

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define AMI_KBDCODE_SELFTESTFAILED 0xFC
#define AMI_KBDCODE_INITKEYSTREAM  0xFD
#define AMI_KBDCODE_ENDKEYSTREAM   0xFE
#define AMI_KBDCODE_LOSTSYNC       0xF9

// Data port
static volatile uint8_t *dPort, *dDir;
static volatile uint8_t dPNum; // Data port pin number

// Clock port
static volatile uint8_t *cPort, *cDir;
static volatile uint8_t cPNum; // Clock port pin number

// Reset port used by A500
static volatile uint8_t *rPort, *rDir;
static volatile uint8_t rPNum; // Reset port pin number

static volatile uint8_t amikbd_synced = 0;

static inline void amikbd_kClock(void);
static inline void amikbd_kToggleData(uint8_t bit);
uint8_t amikbd_kSync(void);

void amikbd_setup(volatile uint8_t *clockPort, volatile uint8_t *clockDir, uint8_t clockPNum, volatile uint8_t *resetPort, volatile uint8_t *resetDir, uint8_t resetPNum) {
#if defined (__AVR_ATmega128__)
	dPort = &PORTD;
	dDir = &DDRD;
	dPNum = 1; // PD1, for INT1
#elif defined (__AVR_ATtiny4313__) || defined (__AVR_ATmega328P__) || defined (__AVR_ATmega8A__)
	dPort = &PORTD;
	dDir = &DDRD;
	dPNum = 3; // PD3, for INT1
#else
	// ???
#endif

	rPort = resetPort;
	rDir = resetDir;
	rPNum = resetPNum;

	cPort = clockPort;
	cDir = clockDir;
	cPNum = clockPNum;

	/* Set all the lines to input and disable pull-up resistors!
	 * The line will be pulled high by the resistors (10K) inside the Amiga. 
	 * When we want to pull a line to low, we simply set the pin to output.
	 */

	// Prepare KDAT port
	*dDir &= ~(1 << dPNum); // KB Data line set as input
	*dPort &= ~(1 << dPNum); // Disable pull-up resistor in data line (Amiga already has one on the line)

	// Prepare KCLK port
	*cDir &= ~(1 << cPNum); // KB Clock line set as input
	*cPort &= ~(1 << cPNum); // Disable pull-up resistor on the clock line

	// Prepare RESET port
	*rDir &= ~(1 << rPNum); // KB reset line set as input
	*rPort &= ~(1 << rPNum); // Disable pull-up resistor in reset line

#if defined (__AVR_ATmega128__) || defined (__AVR_ATmega328P__)
	EICRA &= ~((1 << ISC10) | (1 << ISC11)); // Trigger interrupt at LOW LEVEL (INT1)
	EIMSK &= ~(1 << INT1); // Disable INT1
	EIFR |= (1 << INTF1); // Clear interrupt flag
#elif defined (__AVR_ATtiny4313__)
	MCUCR &= ~((1 << ISC10) | (1 << ISC11)); // Trigger interrupt at LOW LEVEL (INT1)
	GIMSK &= ~(1 << INT1); // Disable INT1
	GIFR |= (1 << INTF1); // Clear interrupt flag
#elif defined (__AVR_ATmega8A__)
	MCUCR &= ~((1 << ISC10) | (1 << ISC11)); // Trigger interrupt at LOW LEVEL (INT1)
	GICR &= ~(1 << INT1); // Disable INT1
	GIFR |= (1 << INTF1); // Clear interrupt flag
#else
	// ???
#endif
}

// http://amigadev.elowar.com/read/ADCD_2.1/Hardware_Manual_guide/node0177.html
void amikbd_init(void) {
	amikbd_kSync();

	// We should send the "test failed" code here, if any problem is detected

	// Send initializate powerup key stream
	amikbd_kSendCommand(AMI_KBDCODE_INITKEYSTREAM);
	// Here we should send the keycodes for all the "pressed" keyboard keys...
	// But I will simply suppose there is none

	amikbd_kSendCommand(AMI_KBDCODE_ENDKEYSTREAM);
	//amikbd_kSendCommand(0x35); // TODO: Check if this is really useless

}

ISR(INT1_vect) { // Manage INT1
	amikbd_synced = 1;

#if defined (__AVR_ATmega128__) || defined (__AVR_ATmega328P__)
	EIMSK &= ~(1 << INT1); // Disable INT1
#elif defined (__AVR_ATtiny4313__)
	GIMSK &= ~(1 << INT1); // Disable INT1
#elif defined (__AVR_ATmega8A__)
	GICR  &= ~(1 << INT1); // Disable INT1
#endif
}

// Clock the keyboard line
static inline void amikbd_kClock(void) {
	_delay_us(20);
	*cDir |= (1 << cPNum); // KB Clock line set as output (thus pulling the line low)
	
	_delay_us(20);
	*cDir &= ~(1 << cPNum); // KB Clock line set as input (thus letting the resistors pull the line high)
	
	_delay_us(20);
}

void amikbd_kForceReset(void) {
	// Pull low the reset line
	*rDir |= (1 << rPNum); // KB reset line set as output (pulling to low)

	// Send a reset signal through the clock port too...
	*cDir |= (1 << cPNum); // KB Clock line set as output (thus pulling the line low)
	_delay_ms(600);
	*cDir &= ~(1 << cPNum); // KB Clock line set as input (thus letting the resistors pull the line high)

	// Set reset line as floating again...
	*rDir &= ~(1 << rPNum); // KB reset line set as input
}

static inline void amikbd_kToggleData(uint8_t bit) {
	if (bit)
		*dDir |= (1 << dPNum); // Set the data pin to output, and pull the line low
	else
		*dDir &= ~(1 << dPNum); // Set the data pin to input, letting the resistor pull the line high

	amikbd_kClock();
}

uint8_t amikbd_kSync(void) {
	uint8_t retries = 0xFF;

#if defined (__AVR_ATmega128__) || defined (__AVR_ATmega328P__)
	EIMSK &= ~(1 << INT1); // Disable INT1
#elif defined (__AVR_ATtiny4313__)
	GIMSK &= ~(1 << INT1); // Disable INT1
#elif defined (__AVR_ATmega8A__)
	GICR  &= ~(1 << INT1); // Disable INT1
#endif
	
	*dDir |= (1 << dPNum); // Set the data pin to output, and pull the line low
	_delay_us(1);
	*dDir &= ~(1 << dPNum); // KB Data line set as input
	_delay_us(20);

#if defined (__AVR_ATmega128__) || defined (__AVR_ATmega328P__)
	EIMSK |= (1 << INT1); // Enable INT1
#elif defined (__AVR_ATtiny4313__)
	GIMSK |= (1 << INT1); // Enable INT1
#elif defined (__AVR_ATmega8A__)
	GICR  |= (1 << INT1); // Enable INT1
#endif
	
	_delay_us(170);

	if (amikbd_synced) return 1; // The keyboard got synced

	while (retries--) {
#if defined (__AVR_ATmega128__) || defined (__AVR_ATmega328P__)
		EIMSK &= ~(1 << INT1); // Disable INT1
#elif defined (__AVR_ATtiny4313__)
		GIMSK &= ~(1 << INT1); // Disable INT1
#elif defined (__AVR_ATmega8A__)
		GICR  &= ~(1 << INT1); // Disable INT1
#endif

		*dDir |= (1 << dPNum); // Set the data pin to output, and pull the line low
		
		amikbd_kClock(); // Send a clock signal

		*dDir &= ~(1 << dPNum); // KB Data line set as input

		_delay_us(20);

#if defined (__AVR_ATmega128__) || defined (__AVR_ATmega328P__)
		EIFR |= (1 << INTF1); // Clear interrupt flag
#elif defined (__AVR_ATtiny4313__) || defined (__AVR_ATmega8A__)
		GIFR |= (1 << INTF1); // Clear interrupt flag
#endif
		amikbd_synced = 0;

#if defined (__AVR_ATmega128__) || defined (__AVR_ATmega328P__)
		EIMSK |= (1 << INT1); // Enable INT1
#elif defined (__AVR_ATtiny4313__) || defined (__AVR_ATmega8A__)
		GICR  |= (1 << INT1); // Enable INT1
#endif
		_delay_us(120);

		if (amikbd_synced) return 1;
	}

	return 0; // Sync failed
}

// http://amigadev.elowar.com/read/ADCD_2.1/Hardware_Manual_guide/node0173.html
void amikbd_kSendCommand(uint8_t command) {
	if (command == 0xFF) return;

	*dDir &= ~(1 << dPNum); // KB Data line set as input, letting the resistor pull the line high

	amikbd_kToggleData((command >> 6) & 1);
	amikbd_kToggleData((command >> 5) & 1);
	amikbd_kToggleData((command >> 4) & 1);
	amikbd_kToggleData((command >> 3) & 1);
	amikbd_kToggleData((command >> 2) & 1);
	amikbd_kToggleData((command >> 1) & 1);
	amikbd_kToggleData((command >> 0) & 1);
	amikbd_kToggleData((command >> 7) & 1);

	amikbd_kSync();
}

