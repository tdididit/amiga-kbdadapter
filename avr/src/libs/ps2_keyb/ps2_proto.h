#ifndef _PS2_PROTO_HEADER_
#define _PS2_PROTO_HEADER_

// http://wiki.altium.com/display/ADOH/PS2+Keyboard+Scan+Codes
#define PS2_SCANCODE_RELEASE 0xF0 // This scancode is sent when releasing a key
#define PS2_SCANCODE_EXTENDED 0xE0
#define PS2_SCANCODE_PAUSE 0xE1
#define PS2_SCANCODE_ACK 0xFA

#define PS2_HTD_LEDCONTROL 0xED
#define PS2_HTD_ALLKEYSMAKEBREAK 0xF8
#define PS2_HTD_RESET 0xFF

#endif /* _PS2_PROTO_HEADER_ */
