#include <stdio.h>
#include <stdint.h>
#include <assert.h>

#include "r2k.h"

unsigned long clock(void);

#if defined(RCM2200)
#define SERIAL_DIVIDER_38400 18
#define CLOCK_DOUBLER 0x07 // clock doubler for 11.0592 MHz base
#define MB0CR_VALUE 0xc8 // What Dynamic C 9 uses for RCM2200 Flash - 0 wait states (but with write-protection added)
#define MB2CR_VALUE 0xc5 // What Dynamic C 9 uses for RCM2200 RAM - 0 wait states
#elif defined(RCM3209)
#define SERIAL_DIVIDER_38400 36
#define CLOCK_DOUBLER 0x03
#define MB0CR_VALUE 0x88 // What Dynamic C 9 uses for RCM3209 Flash - 1 wait state (but with write-protection added)
#endif

_Static_assert((MB0CR_VALUE & 0x07) == 0x00, "Lower bits of Flash Memory Bank Control Register should be compatible with reset value");
_Static_assert((MB2CR_VALUE & 0x0f) == 0x05, "Lower bits of RAM Memory Bank control Register should be the same as in crt0");

void _sdcc_external_startup(void)
{
	// Disable watchdog
	WDTTR = 0x51;
	WDTTR = 0x54;

	// normal oscillator, processor and peripheral from main clock, no periodic interrupt
	GCSR = 0x08;

	GCDR = CLOCK_DOUBLER; // If possible, double clock to get more speed

	// Configure memory wait states
	MB0CR = MB0CR_VALUE;
	MB2CR = MB2CR_VALUE;
}

void init(void)
{
	PCFR = 0x40;                      // Use pin PC6 as TXA
	TAT4R = SERIAL_DIVIDER_38400 - 1; // Use divider for 38400 baud - value in register is one less than the divider used (e.g. a value of 0 will result in clock division by 1).
	TACSR = 0x01;                     // Enable timer A
	SACR = 0x00;                      // No interrupts, 8-bit async mode
}
unsigned long clock(void)
{
	unsigned long clock0, clock1;
	do
	{
		RTC0R = 0;
		clock0 = ((unsigned long)(RTC0R) << 0) | ((unsigned long)(RTC1R) << 8) | ((unsigned long)(RTC2R) << 16) | ((unsigned long)(RTC3R) << 24);
		clock1 = ((unsigned long)(RTC0R) << 0) | ((unsigned long)(RTC1R) << 8) | ((unsigned long)(RTC2R) << 16) | ((unsigned long)(RTC3R) << 24);
	} while (clock0 != clock1);
	return(clock1 / 32.768); // Whetstone assumes CLOCKS_PER_SEC to be 1000.
}

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

