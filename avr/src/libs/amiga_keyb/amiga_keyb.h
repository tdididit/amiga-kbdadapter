#ifndef _AMIGA_KEYBOARD_HEADER_
#define _AMIGA_KEYBOARD_HEADER_

#include <stdint.h>

void amikbd_setup(volatile uint8_t *clockPort, volatile uint8_t *clockDir, uint8_t clockPNum, volatile uint8_t *resetPort, volatile uint8_t *resetDir, uint8_t resetPNum);
void amikbd_init(void);

void amikbd_kSendCommand(uint8_t command); // ANDing the command code with 0x80 sets the release bit
void amikbd_kForceReset(void);

#endif /* _AMIGA_KEYBOARD_HEADER_ */
