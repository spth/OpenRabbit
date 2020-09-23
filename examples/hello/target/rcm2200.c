#include <stdint.h>

#include "../r2k.h"

#define GCDR_VALUE	0x07	// clock doubler for 11.0592 base
#define MB0CR_VALUE	0xC0
#define MB1CR_VALUE	0xC2
#define MB2CR_VALUE	0xC5
#define MB3CR_VALUE	0xC5

// WIP: If we set the MBxCR values elsewhere, we could keep one copy of
// this code.
void dummy(void) {
__asm
target_init::
	ld	a, #MB0CR_VALUE
	ioi
	ld	(_MB0CR), a

	ld	a, #MB1CR_VALUE
	ioi
	ld	(_MB1CR), a
	ld	a, #MB2CR_VALUE
	ioi
	ld	(_MB2CR), a

	ld	a, #MB3CR_VALUE
	ioi
	ld	(_MB3CR), a

	ld	a, #GCDR_VALUE
	ioi
	ld	(_GCDR), a

	jp	init_complete
__endasm;
}

// divider to use for 19200bps
uint8_t divider19200 = 18;
