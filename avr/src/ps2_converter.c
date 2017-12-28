#include "ps2_converter.h"

#include <stdio.h>
#include <avr/pgmspace.h>

#include "amiga_keyb.h"

#include "ps2_proto.h"
#include "ps2_keyb.h"

// PS2 scancodes
// http://www.computer-engineering.org/ps2keyboard/scancodes2.html

// Amiga scancodes
// http://lxr.free-electrons.com/source/drivers/input/keyboard/amikbd.c

// Inspiration...
// https://github.com/ali1234/avr-amiga-controller

#define AMIGA_RESET_CODE 0xFE // This is an 'artificial' code that is not used by normal keypresses. We use it to ask for an amiga reset
#define AMIGA_CAPSLOCK_CODE 0x62 // This is used to manage the difference between PS/2 capslock and the amiga version
#define AMIGA_LCTRL_CODE 0x63
#define AMIGA_LGUI_CODE 0x66
#define AMIGA_RGUI_CODE 0x67

const uint8_t ps2_normal_convtable[256] PROGMEM = {
	0xFF, // 00 
	0x58, // 01 - F9
	0xFF, // 02 
	0x54, // 03 - F5
	0x52, // 04 - F3
	0x50, // 05 - F1
	0x51, // 06 - F2
	0xFF, // 07 - F12 --- Not present in Amiga
	0xFF, // 08
	0x59, // 09 - F10
	0x57, // 0A - F8
	0x55, // 0B - F6
	0x53, // 0C - F4
	0x42, // 0D - 'TAB'
	0x00, // 0E - '`'
	0xFF, // 0F
	0xFF, // 10
	0x64, // 11 - 'LEFT ALT'
	0x60, // 12 - 'LEFT SHIFT'
	0xFF, // 13
	AMIGA_LCTRL_CODE, // 14 - 'LEFT CTRL'
	0x10, // 15 - 'Q'
	0x01, // 16 - '1'
	0xFF, // 17
	0xFF, // 18
	0xFF, // 19
	0x31, // 1A - 'Z'
	0x21, // 1B - 'S'
	0x20, // 1C - 'A'
	0x11, // 1D - 'W'
	0x02, // 1E - '2'
	0xFF, // 1F
	0xFF, // 20
	0x33, // 21 - 'C'
	0x32, // 22 - 'X'
	0x22, // 23 - 'D'
	0x12, // 24 - 'E'
	0x04, // 25 - '4'
	0x03, // 26 - '3'
	0xFF, // 27
	0xFF, // 28
	0x40, // 29 - 'SPACE'
	0x34, // 2A - 'V'
	0x23, // 2B - 'F'
	0x14, // 2C - 'T'
	0x13, // 2D - 'R'
	0x05, // 2E - '5'
	0xFF, // 2F
	0xFF, // 30
	0x36, // 31 - 'N'
	0x35, // 32 - 'B'
	0x25, // 33 - 'H'
	0x24, // 34 - 'G'
	0x15, // 35 - 'Y'
	0x06, // 36 - '6'
	0xFF, // 37
	0xFF, // 38
	0xFF, // 39 
	0x37, // 3A - 'M'
	0x26, // 3B - 'J'
	0x16, // 3C - 'U' 
	0x07, // 3D - '7'
	0x08, // 3E - '8'
	0xFF, // 3F
	0xFF, // 40
	0x38, // 41 - ','
	0x27, // 42 - 'K'
	0x17, // 43 - 'I'
	0x18, // 44 - 'O'
	0x0A, // 45 - '0'
	0x09, // 46 - '9'
	0xFF, // 47
	0xFF, // 48
	0x39, // 49 - '.'
	0x3A, // 4A - '/'
	0x28, // 4B - 'L'
	0x29, // 4C - ';'
	0x19, // 4D - 'P'
	0x0B, // 4E - '-'
	0xFF, // 4F
	0xFF, // 50
	0xFF, // 51
	0x2A, // 52 - '
	0xFF, // 53
	0x1A, // 54 - '['
	0x0C, // 55 - '='
	0xFF, // 56
	0xFF, // 57
	AMIGA_CAPSLOCK_CODE, // 58 - CAPSLOCK
	0x61, // 59 - RIGHT SHIFT
	0x44, // 5A - ENTER
	0x1B, // 5B - ']'
	0xFF, // 5C
	0x0D, // 5D - '\'
	0xFF, // 5E
	0xFF, // 5F
	0xFF, // 60
	0xFF, // 61
	0xFF, // 62
	0xFF, // 63
	0xFF, // 64
	0xFF, // 65
	0x41, // 66 - 'BACKSPACE'
	0xFF, // 67
	0xFF, // 68
	0x1D, // 69 - 'KP 1'
	0xFF, // 6A
	0x2D, // 6B - 'KP 4'
	0x3D, // 6C - 'KP 7'
	0xFF, // 6D
	0xFF, // 6E
	0xFF, // 6F
	0x0F, // 70 - 'KP 0'
	0x3C, // 71 - 'KP .'
	0x1E, // 72 - 'KP 2'
	0x2E, // 73 - 'KP 5'
	0x2F, // 74 - 'KP 6'
	0x3E, // 75 - 'KP 8'
	0x45, // 76 - ESC
	0xFF, // 77 - 'NUM' (Num lock???)
	0xFF, // 78 - F11
	0x5E, // 79 - 'KP +'
	0x1F, // 7A - 'KP 3'
	0x4A, // 7B - 'KP -'
	0x5D, // 7C - 'KP *'
	0x3F, // 7D - 'KP 9'
	0xFF, // 7E - 'SCROLL LOCK' ?
	0xFF, // 7F
	0xFF, // 80
	0xFF, // 81
	0xFF, // 82
	0x56, // 83 - F7
	0xFF, // 84
	0xFF, // 85
	0xFF, // 86
	0xFF, // 87
	0xFF, // 88
	0xFF, // 89
	0xFF, // 8A
	0xFF, // 8B
	0xFF, // 8C
	0xFF, // 8D
	0xFF, // 8E
	0xFF, // 8F
	0xFF, // 90
	0xFF, // 91
	0xFF, // 92
	0xFF, // 93
	0xFF, // 94
	0xFF, // 95
	0xFF, // 96
	0xFF, // 97
	0xFF, // 98
	0xFF, // 99
	0xFF, // 9A
	0xFF, // 9B
	0xFF, // 9C
	0xFF, // 9D
	0xFF, // 9E
	0xFF, // 9F
	0xFF, // A0
	0xFF, // A1
	0xFF, // A2
	0xFF, // A3
	0xFF, // A4
	0xFF, // A5
	0xFF, // A6
	0xFF, // A7
	0xFF, // A8
	0xFF, // A9
	0xFF, // AA
	0xFF, // AB
	0xFF, // AC
	0xFF, // AD
	0xFF, // AE
	0xFF, // AF
	0xFF, // B0
	0xFF, // B1
	0xFF, // B2
	0xFF, // B3
	0xFF, // B4
	0xFF, // B5
	0xFF, // B6
	0xFF, // B7
	0xFF, // B8
	0xFF, // B9
	0xFF, // BA
	0xFF, // BB
	0xFF, // BC
	0xFF, // BD
	0xFF, // BE
	0xFF, // BF
	0xFF, // C0
	0xFF, // C1
	0xFF, // C2
	0xFF, // C3
	0xFF, // C4
	0xFF, // C5
	0xFF, // C6
	0xFF, // C7
	0xFF, // C8
	0xFF, // C9
	0xFF, // CA
	0xFF, // CB
	0xFF, // CC
	0xFF, // CD
	0xFF, // CE
	0xFF, // CF
	0xFF, // D0
	0xFF, // D1
	0xFF, // D2
	0xFF, // D3
	0xFF, // D4
	0xFF, // D5
	0xFF, // D6
	0xFF, // D7
	0xFF, // D8
	0xFF, // D9
	0xFF, // DA
	0xFF, // DB
	0xFF, // DC
	0xFF, // DD
	0xFF, // DE
	0xFF, // DF
	0xFF, // E0
	0xFF, // E1
	0xFF, // E2
	0xFF, // E3
	0xFF, // E4
	0xFF, // E5
	0xFF, // E6
	0xFF, // E7
	0xFF, // E8
	0xFF, // E9
	0xFF, // EA
	0xFF, // EB
	0xFF, // EC
	0xFF, // ED
	0xFF, // EE
	0xFF, // EF
	0xFF, // F0
	0xFF, // F1
	0xFF, // F2
	0xFF, // F3
	0xFF, // F4
	0xFF, // F5
	0xFF, // F6
	0xFF, // F7
	0xFF, // F8
	0xFF, // F9
	0xFF, // FA
	0xFF, // FB
	0xFF, // FC
	0xFF, // FD
	0xFF, // FE
	0xFF  // FF
};

const uint8_t ps2_extended_convtable[256] PROGMEM = {
	0xFF, // 00 
	0xFF, // 01
	0xFF, // 02
	0xFF, // 03
	0xFF, // 04
	0xFF, // 05
	0xFF, // 06
	0xFF, // 07
	0xFF, // 08
	0xFF, // 09
	0xFF, // 0A
	0xFF, // 0B
	0xFF, // 0C
	0xFF, // 0D
	0xFF, // 0E
	0xFF, // 0F
	0xFF, // 10
	0x65, // 11 - 'RIGHT ALT'
	0xFF, // 12
	0xFF, // 13
	0x63, // 14 - 'RIGHT CTRL'
	0xFF, // 15
	0xFF, // 16
	0xFF, // 17
	0xFF, // 18
	0xFF, // 19
	0xFF, // 1A
	0xFF, // 1B
	0xFF, // 1C
	0xFF, // 1D
	0xFF, // 1E
	AMIGA_LGUI_CODE, // 1F - 'LEFT GUI' (Windows button?)
	0xFF, // 20
	0xFF, // 21
	0xFF, // 22
	0xFF, // 23
	0xFF, // 24
	0xFF, // 25
	0xFF, // 26
	AMIGA_RGUI_CODE, // 27 - 'RIGHT GUI' (Windows button?)
	0xFF, // 28
	0xFF, // 29
	0xFF, // 2A
	0xFF, // 2B
	0xFF, // 2C
	0xFF, // 2D
	0xFF, // 2E
	0xFF, // 2F - 'APPS' ????
	0xFF, // 30
	0xFF, // 31
	0xFF, // 32
	0xFF, // 33
	0xFF, // 34
	0xFF, // 35
	0xFF, // 36
	0xFF, // 37
	0xFF, // 38
	0xFF, // 39 
	0xFF, // 3A
	0xFF, // 3B
	0xFF, // 3C
	0xFF, // 3D
	0xFF, // 3E
	0xFF, // 3F
	0xFF, // 40
	0xFF, // 41
	0xFF, // 42
	0xFF, // 43
	0xFF, // 44
	0xFF, // 45
	0xFF, // 46
	0xFF, // 47
	0xFF, // 48
	0xFF, // 49
	0x5C, // 4A - 'KP /'
	0xFF, // 4B
	0xFF, // 4C
	0xFF, // 4D
	0xFF, // 4E
	0xFF, // 4F
	0xFF, // 50
	0xFF, // 51
	0xFF, // 52
	0xFF, // 53
	0xFF, // 54
	0xFF, // 55
	0xFF, // 56
	0xFF, // 57
	0xFF, // 58
	0xFF, // 59
	0x43, // 5A - 'KP ENTER'
	0xFF, // 5B
	0xFF, // 5C
	0xFF, // 5D
	0xFF, // 5E
	0xFF, // 5F
	0xFF, // 60
	0xFF, // 61
	0xFF, // 62
	0xFF, // 63
	0xFF, // 64
	0xFF, // 65
	0xFF, // 66
	0xFF, // 67
	0xFF, // 68
	AMIGA_RESET_CODE, // 69 - 'END' // *** Use it as reset button???
	0xFF, // 6A
	0x4F, // 6B - 'LEFT ARROW'
	0x5F, // 6C - 'HOME' // Used as HELP button
	0xFF, // 6D
	0xFF, // 6E
	0xFF, // 6F
	0xFF, // 70 - 'INSERT'
	0xFF, // 71 - 'DELETE'
	0x4D, // 72 - 'DOWN ARROW'
	0xFF, // 73
	0x4E, // 74 - 'RIGHT ARROW'
	0x4C, // 75 - 'UP ARROW'
	0xFF, // 76
	0xFF, // 77
	0xFF, // 78
	0xFF, // 79
	0xFF, // 7A - 'PAD DOWN'
	0xFF, // 7B
	0xFF, // 7C
	0xFF, // 7D - 'PAG UP'
	0xFF, // 7E
	0xFF, // 7F
	0xFF, // 80
	0xFF, // 81
	0xFF, // 82
	0xFF, // 83
	0xFF, // 84
	0xFF, // 85
	0xFF, // 86
	0xFF, // 87
	0xFF, // 88
	0xFF, // 89
	0xFF, // 8A
	0xFF, // 8B
	0xFF, // 8C
	0xFF, // 8D
	0xFF, // 8E
	0xFF, // 8F
	0xFF, // 90
	0xFF, // 91
	0xFF, // 92
	0xFF, // 93
	0xFF, // 94
	0xFF, // 95
	0xFF, // 96
	0xFF, // 97
	0xFF, // 98
	0xFF, // 99
	0xFF, // 9A
	0xFF, // 9B
	0xFF, // 9C
	0xFF, // 9D
	0xFF, // 9E
	0xFF, // 9F
	0xFF, // A0
	0xFF, // A1
	0xFF, // A2
	0xFF, // A3
	0xFF, // A4
	0xFF, // A5
	0xFF, // A6
	0xFF, // A7
	0xFF, // A8
	0xFF, // A9
	0xFF, // AA
	0xFF, // AB
	0xFF, // AC
	0xFF, // AD
	0xFF, // AE
	0xFF, // AF
	0xFF, // B0
	0xFF, // B1
	0xFF, // B2
	0xFF, // B3
	0xFF, // B4
	0xFF, // B5
	0xFF, // B6
	0xFF, // B7
	0xFF, // B8
	0xFF, // B9
	0xFF, // BA
	0xFF, // BB
	0xFF, // BC
	0xFF, // BD
	0xFF, // BE
	0xFF, // BF
	0xFF, // C0
	0xFF, // C1
	0xFF, // C2
	0xFF, // C3
	0xFF, // C4
	0xFF, // C5
	0xFF, // C6
	0xFF, // C7
	0xFF, // C8
	0xFF, // C9
	0xFF, // CA
	0xFF, // CB
	0xFF, // CC
	0xFF, // CD
	0xFF, // CE
	0xFF, // CF
	0xFF, // D0
	0xFF, // D1
	0xFF, // D2
	0xFF, // D3
	0xFF, // D4
	0xFF, // D5
	0xFF, // D6
	0xFF, // D7
	0xFF, // D8
	0xFF, // D9
	0xFF, // DA
	0xFF, // DB
	0xFF, // DC
	0xFF, // DD
	0xFF, // DE
	0xFF, // DF
	0xFF, // E0
	0xFF, // E1
	0xFF, // E2
	0xFF, // E3
	0xFF, // E4
	0xFF, // E5
	0xFF, // E6
	0xFF, // E7
	0xFF, // E8
	0xFF, // E9
	0xFF, // EA
	0xFF, // EB
	0xFF, // EC
	0xFF, // ED
	0xFF, // EE
	0xFF, // EF
	0xFF, // F0
	0xFF, // F1
	0xFF, // F2
	0xFF, // F3
	0xFF, // F4
	0xFF, // F5
	0xFF, // F6
	0xFF, // F7
	0xFF, // F8
	0xFF, // F9
	0xFF, // FA
	0xFF, // FB
	0xFF, // FC
	0xFF, // FD
	0xFF, // FE
	0xFF  // FF
};

void ps2k_callback(uint8_t *code, uint8_t count) {
	static uint8_t amiga_reset_sequence = 0x00; // This byte is used to keep track
	static uint8_t old_amiga_scancode = 0xFF;
	static uint8_t amiga_capslock_pressed = 0;

	uint8_t amiga_scancode = 0;
	uint8_t ps2_led_command[] = {PS2_HTD_LEDCONTROL, 0x00};

	if (count == 0) { // Normal key pressed
		amiga_scancode = pgm_read_byte(&ps2_normal_convtable[code[0]]);

		if (amiga_scancode == AMIGA_LCTRL_CODE) { // Keep track of the key for the reset sequence
			amiga_reset_sequence |= 0xE0;
		}
	} else if (count == 1 && code[0] == PS2_SCANCODE_RELEASE) { // Normal key depressed
		amiga_scancode = pgm_read_byte(&ps2_normal_convtable[code[1]]) | 0x80;

		if (amiga_scancode == (AMIGA_LCTRL_CODE | 0x80)) { // Keep track of the key for the reset sequence
			amiga_reset_sequence &= 0x1F;
		}
	} else if (count == 1 && code[0] == PS2_SCANCODE_EXTENDED) { // Extended key pressed
		amiga_scancode = pgm_read_byte(&ps2_extended_convtable[code[1]]);

		switch (amiga_scancode) { // Keep track of the key for the reset sequence
			case AMIGA_LGUI_CODE:
				amiga_reset_sequence |= 0x1C;
				break;
			case AMIGA_RGUI_CODE:
				amiga_reset_sequence |= 0x03;
				break;
			default:
				break;
		}
	} else if (count == 2) { // Extended key depressed
		amiga_scancode = pgm_read_byte(&ps2_extended_convtable[code[2]]) | 0x80;				
		
		switch (amiga_scancode) { // Keep track of the key for the reset sequence
			case (AMIGA_LGUI_CODE | 0x80):
				amiga_reset_sequence &= 0xE3;
				break;
			case (AMIGA_RGUI_CODE | 0x80):
				amiga_reset_sequence &= 0xFC;
				break;
			default:
				break;
		}
	} else {
		old_amiga_scancode = amiga_scancode;
		return;
	}

	if (amiga_reset_sequence == 0xFF) { // Reset sequence completed
		amiga_scancode = AMIGA_RESET_CODE; // Force a reset!
	}

	if (amiga_scancode == AMIGA_RESET_CODE) {
		amiga_reset_sequence = 0x00;
		
		amikbd_kForceReset(); // Force a reset on the Amiga
				
		ps2_led_command[0] = 0xFF; // Reset the keyboard
		ps2keyb_sendCommand(ps2_led_command, 1);
		amiga_capslock_pressed = 0;
	} else if ((amiga_scancode != old_amiga_scancode) && (amiga_scancode != 0xFF)) {
		if (amiga_scancode == AMIGA_CAPSLOCK_CODE) { // We need to manage the capslock differently: on the amiga it remains pressed until someone pushes it again
			if (!amiga_capslock_pressed) { // The capslock wasn't pressed. Treat the key normally
				amiga_capslock_pressed = 1;
				amikbd_kSendCommand(AMIGA_CAPSLOCK_CODE);

				ps2_led_command[1] = 0x04; // Turn ON caps lock led
				ps2keyb_sendCommand(ps2_led_command, 2);
			} else { // Release the capslock
				amiga_capslock_pressed = 0;
				amikbd_kSendCommand(AMIGA_CAPSLOCK_CODE | 0x80);

				ps2_led_command[1] = 0x00; // Turn OFF caps lock led
				ps2keyb_sendCommand(ps2_led_command, 2);
			}
		} else if (amiga_scancode != (AMIGA_CAPSLOCK_CODE | 0x80)) { // Every other key, except the capslock release, which we ignore
			amikbd_kSendCommand(amiga_scancode);
		}
	}

	old_amiga_scancode = amiga_scancode;
}
