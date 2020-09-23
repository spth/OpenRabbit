#include <stdint.h>
#include <stdio.h>

#include "r2k_reg.h"

extern uint8_t divider19200;

int putchar(int c)
{
	// Convert newline to CRLF
	if (c == '\n') {
		putchar('\r');
	}

	while (SASR & 0x04);	// Wait for empty transmitter data register
	SADR = c;
	return c;
}

void main(void)
{
#if 0	// RCM2200
	PCDR = 0x01;
	PECR = 0x82;
	PEDDR = 0x82;
	PEDR = 0x80;
#else	// RCM3209
	PGCR = 0x00;
	PGFR = 0x00;
	PGDCR = 0xC0;
	PGDR = 0x80;	// bit 6 and 7 set to 0 for LED ON
	PGDDR = 0xC4;
#endif

	PCFR = 0x40;	// Use pin PC6 as TXA

	TAT4R = divider19200;	// use divider for 19200
	TACSR = 0x01;	// Enable timer A

	SACR = 0x00;	// No interrupts, 8-bit async mode

	for (;;) {
		printf("Hello, world!\n");
	}
}

