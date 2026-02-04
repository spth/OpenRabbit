
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

#if defined(RCM2000)       // RCM2000: 25.8 MHz, 256K flash, 512K SRAM.
#define SERIAL_DIVIDER_38400 21
#define CLOCK_DOUBLER 0x05 // Clock doubler for 12.9024 MHz base
#define MB0CR_VALUE 0xc8   // Flash - 0 wait states (for 45 ns Flash @ 25.49 MHz) with write-protection
#define MB1CR_VALUE 0xc8   // Flash - 0 wait states (for 45 ns Flash @ 25.49 MHz) with write-protection
#define MB2CR_VALUE 0x85   // RAM - 1 wait state (for 70 ns RAM @ 25.49 MHz)
#define MB3CR_VALUE 0x85   // RAM - 1 wait state (for 70 ns RAM @ 25.49 MHz)

#elif defined(RCM2010)     // RCM2010: 25.8 MHz, 256K flash, 128K SRAM.
// TODO

#elif defined(RCM2020)     // RCM2020: 18.4 MHz, 256K flash, 128K SRAM.
#define SERIAL_DIVIDER_38400 15
#define CLOCK_DOUBLER 0x07 // Clock doubler for 9.216 MHz base
#define MB0CR_VALUE 0xc8   // Flash - 0 wait states with write-protection
#define MB1CR_VALUE 0xc8   // Flash - 0 wait states with write-protection
#define MB2CR_VALUE 0xc5   // RAM - 0 wait states

#elif defined(RCM2100)     // RCM2100: 22.1 MHz, 512K Flash, 512K SRAM
// TODO

#elif defined(RCM2110)     // RCM2110: 22.1 MHz, 128K Flash, 256K SRAM
// TODO

#elif defined(RCM2120)     // RCM2120: 22.1 MHz, 512K Flash, 512K SRAM
// TODO

#elif defined(RCM2130)     // RCM2130: 22.1 MHz, 128K Flash, 256K SRAM
// TODO

#elif defined(RCM2200)     // RCM2200: 22.1 MHz, 256K flash, 128K SRAM
#define SERIAL_DIVIDER_38400 18
#define CLOCK_DOUBLER 0x07 // Clock doubler for 11.0592 MHz base
#define MB0CR_VALUE 0xc8   // Flash - 0 wait states (for 45 ns Flash @ 22.12 MHz) with write-protection
#define MB2CR_VALUE 0xc5   // RAM - 0 wait states (for 55 ns RAM @ 22.12 MHz)

#elif defined(RCM2210)     // RCM2210: 22.12 MHz, 256K flash, 128K SRAM
#define SERIAL_DIVIDER_38400 18
#define CLOCK_DOUBLER 0x07 // Clock doubler for 11.0592 MHz base
#define MB0CR_VALUE 0xc8   // Flash - 0 wait states (for 45 ns Flash @ 22.12 MHz) with write-protection
#define MB2CR_VALUE 0xc5   // RAM - 0 wait states (for 55 ns RAM @ 22.12 MHz)

#elif defined(RCM2250)     // RCM2200: 22.1 MHz, 512K flash, 512K SRAM
#define SERIAL_DIVIDER_38400 18
#define CLOCK_DOUBLER 0x07 // Clock doubler for 11.0592 MHz base
#define MB0CR_VALUE 0xc8   // Flash - 0 wait states with write-protection
#define MB1CR_VALUE 0xc8   // Flash - 0 wait states with write-protection
#define MB2CR_VALUE 0xc5   // RAM - 0 wait states
#define MB3CR_VALUE 0xc5   // RAM - 0 wait states

#elif defined(RCM2260)     // RCM2200: 22.1 MHz, 512K flash, 512K SRAM
// TODO

#elif defined(RCM3000)     // RCM3000: 29.4 MHz, 512K flash, 512K SRAM
#define SERIAL_DIVIDER_38400 24
#define CLOCK_DOUBLER 0x07 // Clock doubler for 14.7456 MHz base
// TODO

#elif defined(RCM3010)     // RCM3010: 29.4 MHz, 256K flash, 128K SRAM
#define SERIAL_DIVIDER_38400 24
#define CLOCK_DOUBLER 0x07 // Clock doubler for 14.7456 MHz base
#define MB0CR_VALUE 0xc8   // Flash 0 wait states (guess, as the values in the id block don't make sense) with write-protection
#define MB2CR_VALUE 0xc5   // RAM - 0 wait states (guess, as the values in the id block don't make sense)

#elif defined(RCM3100)     // RCM3100: 29.4 MHz, 512K flash, 512K SRAM
#define SERIAL_DIVIDER_38400 24
#define CLOCK_DOUBLER 0x07 // Clock doubler for 14.7456 MHz base
// TODO

#elif defined(RCM3110)     // RCM3110: 29.49 MHz, 256K flash, 128K SRAM
#define SERIAL_DIVIDER_38400 24
#define CLOCK_DOUBLER 0x07 // Clock doubler for 14.7456 MHz base
#define MB0CR_VALUE 0xc8   // Flash 0 wait states (for 45 ns Flash @ 29.49 MHz) with write-protection
#define MB2CR_VALUE 0xc5   // RAM - 0 wait states (for 55 ns RAM @ 29.49 MHz)

#elif defined(RCM3209)     // RCM3209: 44.2 MHz, 256K flash, 256K SRAM
#define SERIAL_DIVIDER_38400 36
#define CLOCK_DOUBLER 0x03 // Clock doubler for 22.116 MHz base
#define MB0CR_VALUE 0x88   // What Dynamic C 9 uses for RCM3209 Flash - 1 wait state (but with write-protection added)
#define MB1CR_VALUE 0x88   // What Dynamic C 9 uses for RCM3209 Flash - 1 wait state (but with write-protection added)
#define MB2CR_VALUE 0x85   // RAM - 1 wait state (for 55 ns RAM @ 44.2 MHz)

#elif defined(RCM3300)     // RCM3300: 44.2 MHz, 512K flash, 1MB SRAM
#define SERIAL_DIVIDER_38400 36
#define CLOCK_DOUBLER 0x03 // Clock doubler for 22.116 MHz base
// TODO

#elif defined(RCM3309)     // RCM3309: 44.2 MHz, 512K flash, 512K SRAM
#define SERIAL_DIVIDER_38400 36
#define CLOCK_DOUBLER 0x03 // Clock doubler for 22.116 MHz base
// TODO

#elif defined(RCM3310)     // RCM3310: 44.2 MHz, 512K flash, 1MB SRAM
#define SERIAL_DIVIDER_38400 36
#define CLOCK_DOUBLER 0x03 // Clock doubler for 22.116 MHz base
#define MB0CR_VALUE 0x88   // Flash - 1 wait state (for 45 ns Flash @ 44.2 MHz) with write-protection
#define MB1CR_VALUE 0x88   // Flash - 1 wait state (for 45 ns Flash @ 44.2 MHz) with write-protection
#define MB2CR_VALUE 0x85   // RAM - 1 wait states (for 55 ns RAM @ 44.2 MHz)
#define MB3CR_VALUE 0x85   // RAM - 1 wait states (for 55 ns RAM @ 44.2 MHz)

#elif defined(RCM3319)     // RCM3319: 44.2 MHz, 512K flash, 512K SRAM
#define SERIAL_DIVIDER_38400 36
#define CLOCK_DOUBLER 0x03 // Clock doubler for 22.116 MHz base
#define MB0CR_VALUE 0x88   // Flash - 1 wait state (for 45 ns Flash @ 44.2 MHz) with write-protection
#define MB1CR_VALUE 0x88   // Flash - 1 wait state (for 45 ns Flash @ 44.2 MHz) with write-protection
#define MB2CR_VALUE 0x85   // RAM - 1 wait states (for 55 ns RAM @ 44.2 MHz)
#define MB3CR_VALUE 0x85   // RAM - 1 wait states (for 55 ns RAM @ 44.2 MHz)

#elif defined(RCM3360)     // RCM3360: 44.2 MHz, 512K flash, 1MB SRAM
// TODO

#elif defined(RCM3370)     // RCM3370: 44.2 MHz, 512K flash, 1MB SRAM
// TODO

#elif defined(RCM3400)     // RCM3400: 29.4 MHz, 512K flash, 512K SRAM
// TODO

#elif defined(RCM3410)     // RCM3410: 29.4 MHz, 256K flash, 256K SRAM
// TODO

#elif defined(RCM3600)     // RCM3600: 22.1 MHz, 512K flash, 512K SRAM
// TODO

#elif defined(RCM3610)     // RCM3610: 22.1 MHz, 256K flash, 128K SRAM
// TODO

#elif defined(RCM3700)     // RCM3700: 22.1 MHz, 512K flash, 512K SRAM
// TODO

#elif defined(RCM3710)     // RCM3710: 22.1 MHz, 256K flash, 128K SRAM
#define SERIAL_DIVIDER_38400 18
#define CLOCK_DOUBLER 0x07 // Clock doubler for 11.0592 MHz base
#define MB0CR_VALUE 0xc8   // Flash - 0 wait states (for 45 ns Flash @ 22.1 MHz) with write-protection
#define MB2CR_VALUE 0xc5   // RAM - 0 wait states (for 55 ns RAM @ 22.1 MHz)

#elif defined(RCM3720)     // RCM3720: 22.1 MHz, 512K flash, 256K SRAM
#define SERIAL_DIVIDER_38400 18
#define CLOCK_DOUBLER 0x07 // Clock doubler for 11.0592 MHz base
#define MB0CR_VALUE 0xc8   // Flash - 0 wait states (for 45 ns Flash @ 22.1 MHz) with write-protection
#define MB1CR_VALUE 0xc8   // Flash - 0 wait states (for 45 ns Flash @ 22.1 MHz) with write-protection
#define MB2CR_VALUE 0xc5   // RAM - 0 wait states (for 55 ns RAM @ 22.1 MHz)

#elif defined(RCM3750)     // RCM3750: 22.1 MHz, 512K flash, 512K SRAM
#define SERIAL_DIVIDER_38400 18
#define CLOCK_DOUBLER 0x07 // Clock doubler for 11.0592 MHz base
#define MB0CR_VALUE 0xc8   // Flash - 0 wait states with write-protection
#define MB1CR_VALUE 0xc8   // Flash - 0 wait states with write-protection
#define MB2CR_VALUE 0xc5   // RAM - 0 wait states
#define MB3CR_VALUE 0xc5   // RAM - 0 wait states

#elif defined(RCM3800)     // RCM3800: 51.61 MHz, 512K flash, 256K SRAM
#define SERIAL_DIVIDER_38400 42
#define CLOCK_DOUBLER 0x03 // Clock doubler for 25.8048 MHz base
#define MB0CR_VALUE 0x48   // Flash - 2 wait states (for 45 ns Flash @ 51.61 MHz) with write-protection
#define MB1CR_VALUE 0x48   // Flash - 2 wait states (for 45 ns Flash @ 51.61 MHz) with write-protection
#define MB2CR_VALUE 0xc5   // RAM - 0 wait states (for 15 ns RAM @ 51.61 MHz)

#elif defined(RCM3900)     // RCM3900: 44.2 MHz, 512K flash, 512K SRAM
// TODO

#elif defined(RCM3910)     // RCM3910: 44.2 MHz, 512K flash, 512K SRAM
// TODO

#elif defined(RCM4000)     // RCM4000: 58.98 MHz, 512K flash (16-bit), 512K SRAM (16-bit)
// TODO

#elif defined(RCM4010)     // RCM4010: 58.98 MHz, 512K flash (16-bit), 512K SRAM (16-bit)
// TODO

#elif defined(RCM4050)     // RCM4050: 58.98 MHz, 1MB flash (16-bit), 1MB SRAM (16-bit)
// TODO

#elif defined(RCM4100)     // RCM4100: 58.98 MHz, 512K flash, 512K SRAM
// TODO

#elif defined(RCM4100)     // RCM4100: 58.98 MHz, 512K flash, 512K SRAM
// TODO

#elif defined(RCM4110)     // RCM4110: 29.49 MHz, 512K flash, 256K SRAM
#define SERIAL_DIVIDER_38400 24
#define CLOCK_DOUBLER 0x00 // Crystal runs at full target speed, no clock doubler needed
#define MB0CR_VALUE 0xc8   // Flash 0 wait states with write-protection (guess, as the values in the id block don't make sense)
#define MB1CR_VALUE 0xc8   // Flash 0 wait states with write-protection (guess, as the values in the id block don't make sense)
#define MB2CR_VALUE 0xc5   // RAM - 0 wait states (guess, as the values in the id block don't make sense)

#elif defined(RCM4120)     // RCM4120: 58.98 MHz, 512K flash, 512K SRAM
// TODO

#elif defined(RCM4200)     // RCM4200: 58.98 MHz, 512K flash, 512K SRAM
// TODO

#elif defined(RCM4210)     // RCM4200: 29.49 MHz, 512K flash, 512K SRAM
#define SERIAL_DIVIDER_38400 24
#define CLOCK_DOUBLER 0x00 // Crystal runs at full target speed, no clock doubler needed
#define MB0CR_VALUE 0xc8   // Flash - 1 wait state (for 70 ns Flash @ 29.49 MHz) with write-protection
#define MB1CR_VALUE 0xc8   // Flash - 1 wait state (for 70 ns Flash @ 29.49 MHz) with write-protection
#define MB2CR_VALUE 0xc5   // RAM - 0 wait states (for 55 ns RAM @ 29.49 MHz)
#define MB3CR_VALUE 0xc5   // RAM - 0 wait states (for 55 ns RAM @ 29.49 MHz)

#elif defined(RCM4310)     // RCM4310: 58.98 MHz, 1024K flash, 512K SRAM
// TODO

#elif defined(RCM4310)     // RCM4310: 58.98 MHz, 512K flash, 512K SRAM
// TODO

#elif defined(RCM5400W)    // RCM5400W: 73.73 MHz, 512K flash, 512K RAM
// TODO

#elif defined(RCM5450W)    // RCM5450W: 73.73 MHz, 1024K flash, 512K RAM
// TODO

#elif defined(RCM5600W)    // RCM5600W: 73.73 MHz, 1024K flash, 1024K RAM
// TODO

#elif defined(RCM5600W)    // RCM5650W: 73.73 MHz, 4096K flash, 1024K RAM
// TODO

#elif defined(RCM5700)     // RCM5700: 50.00 MHz, 1024K flash, 128K RAM
#define SERIAL_DIVIDER_38400 41
#define CLOCK_DOUBLER 0x07 // Clock doubler for 25.000 MHz base
#define MB0CR_VALUE 0x08   // Flash - 4 wait states (3 would do for 70 ns Flash @ 50.00 MHz, but 3 is not a possible setting) with write-protection
#define MB2CR_VALUE 0xc5   // RAM - 0 wait states (for 15 ns RAM @ 50.00 MHz)

#elif defined(RCM5710)     // RCM5710: 50.00 MHz, 1024K flash, 128K RAM
// TODO

#elif defined(RCM5750)     // RCM5710: 50.00 MHz, 1024K flash, 640K RAM
// TODO

#elif defined(RCM5760)     // RCM5710: 50.00 MHz, 1024K flash, 640K RAM
// TODO

#elif defined(RCM6700)     // RCM6700: 162.5 MHz, 1024K flash, 1024K RAM
// TODO

#elif defined(RCM6710)     // RCM6710: 162.5 MHz, 1024K flash, 1024K RAM
// TODO

#elif defined(RCM6750)     // RCM6750: 162.5 MHz, 4096K flash, 1024K RAM
// TODO

#elif defined(RCM6760)     // RCM6760: 162.5 MHz, 4096K flash, 1024K RAM
// TODO

#endif

_Static_assert((MB0CR_VALUE & 0x07) == 0x00, "Lower bits of Flash Memory Bank Control Register should be compatible with reset value");
_Static_assert((MB2CR_VALUE & 0x0f) == 0x05, "Lower bits of RAM Memory Bank control Register should be the same as in crt0");

