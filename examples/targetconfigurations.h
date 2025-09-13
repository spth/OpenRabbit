
// The configuration values MB0CR_VALUE for Flash and MB2CR_VALUE for RAM can be calculated from:
// Flash / RAM speed (displayed by openrabbit when using --verbose)
// µC Specification (see section on AC timing specification or memory access times in the respective Rabbit user manual)
//
// TODO:
// For some RCM, the 45 ns flash used initially was replaced by 55 ns flash, causing a reliability issue on boards
// With a 29 MHz crystal when the clock doubler is used.
// RCM3000 Rev F (and later)
// RCM3010 Rev F (and later)
// RCM3100 Rev D (and later)
// RCM3110 Rev D (and later)
// RCM3400 Rev D (and later)
// For these, timing margins on OE are very narrow, resulting in unreliable operation.
// This can be worked around by enabling early OE in MTCR before enabling the clock doubler.
// Later, some Digi customers reported seeing the same issue on an RCM with 22.1 Mhz clock (RCM 3365) when using the
// clock doubler at elevated temperatures and executing code from RAM at 0 wait states.

#if defined(RCM2000)       // RCM2000: 25.8 MHz
#define SERIAL_DIVIDER_38400 21
#define CLOCK_DOUBLER 0x05 // Clock doubler for 12.9024 MHz base
#define MB0CR_VALUE 0xc8   // Flash - 0 wait states (for 45 ns Flash @ 25.49 MHz) with write-protection
#define MB2CR_VALUE 0x85   // RAM - 1 wait state (for 70 ns RAM @ 25.49 MHz)

#elif defined(RCM2020)       // RCM2020: 18.4 MHz
#define SERIAL_DIVIDER_38400 15
#define CLOCK_DOUBLER 0x07 // Clock doubler for 9.216 MHz base
#define MB0CR_VALUE 0xc8   // Flash - 0 wait states with write-protection
#define MB2CR_VALUE 0xc5   // RAM - 0 wait states

#elif defined(RCM2200)     // RCM2200: 22.1 MHz
#define SERIAL_DIVIDER_38400 18
#define CLOCK_DOUBLER 0x07 // Clock doubler for 11.0592 MHz base
#define MB0CR_VALUE 0xc8   // Flash - 0 wait states with write-protection
#define MB2CR_VALUE 0xc5   // RAM - 0 wait states

#elif defined(RCM3110)     // RCM3110: 29.49 MHz
#define SERIAL_DIVIDER_38400 24
#define CLOCK_DOUBLER 0x07 // Clock doubler for 14.7456 MHz base
#define MB0CR_VALUE 0xc8   // Flash 0 wait states (for 45 ns Flash @ 29.49 MHz) with write-protection
#define MB2CR_VALUE 0xc5   // RAM - 0 wait states (for 55 ns RAM @ 29.49 MHz)

#elif defined(RCM3209)     // RCM3209: 44.2 MHz
#define SERIAL_DIVIDER_38400 36
#define CLOCK_DOUBLER 0x03 // Clock doubler for 22.116 MHz base
#define MB0CR_VALUE 0x88   // What Dynamic C 9 uses for RCM3209 Flash - 1 wait state (but with write-protection added)
#define MB2CR_VALUE 0x85   // RAM - 1 wait state (for 55 ns RAM @ 44.2 MHz)

#elif defined(RCM3319)     // RCM3319: 44.2 MHz
#define SERIAL_DIVIDER_38400 36
#define CLOCK_DOUBLER 0x03 // Clock doubler for 22.116 MHz base
#define MB0CR_VALUE 0x88   // Flash - 1 wait state (for 45 ns Flash @ 44.2 MHz) with write-protection
#define MB2CR_VALUE 0x85   // RAM - 1 wait states (for 55 ns RAM @ 44.2 MHz)

#elif defined(RCM3750)     // RCM3750: 22.1 MHz
#define SERIAL_DIVIDER_38400 18
#define CLOCK_DOUBLER 0x07 // Clock doubler for 11.0592 MHz base
#define MB0CR_VALUE 0xc8   // Flash - 0 wait states with write-protection
#define MB2CR_VALUE 0xc5   // RAM - 0 wait states

#elif defined(RCM4110)     // RCM4110: 29.49 MHz
#define SERIAL_DIVIDER_38400 24
#define CLOCK_DOUBLER 0x00 // Crystal runs at full target speed, no clock doubler needed
#define MB0CR_VALUE 0xc8   // Flash 0 wait states with write-protection (guess, as the values in the id block don't make sense)
#define MB2CR_VALUE 0xc5   // RAM - 0 wait states (guess, as the values in the id block don't make sense)

#elif defined(RCM5700)     // RCM5700: 50.00 MHz
#define SERIAL_DIVIDER_38400 41
#define CLOCK_DOUBLER 0x07 // Clock doubler for 25.000 MHz base
#define MB0CR_VALUE 0x08   // Flash - 4 wait states (3 would do for 70 ns Flash @ 50.00 MHz, but 3 is not a possible setting) with write-protection
#define MB2CR_VALUE 0xc5   // RAM - 0 wait states (for 15 ns RAM @ 50.00 MHz)

#endif

_Static_assert((MB0CR_VALUE & 0x07) == 0x00, "Lower bits of Flash Memory Bank Control Register should be compatible with reset value");
_Static_assert((MB2CR_VALUE & 0x0f) == 0x05, "Lower bits of RAM Memory Bank control Register should be the same as in crt0");

