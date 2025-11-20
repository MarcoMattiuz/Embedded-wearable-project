#include <stdint.h>

/*
   Take 'A' as example.
   'A' use 5 byte to denote:
       0x7C, 0x12, 0x11, 0x12, 0x7C

   and we represent it in base 2:
       0x7C: 01111100
       0x12: 00010010
       0x11: 00010001
       0x12: 00010010
       0x7C: 01111100
   where 1 is font color, and 0 is background color

   So it's 'A' if we look it in counter-clockwise for 90 degree.
   In general case, we also add a background line to seperate from other character:
       0x7C: 01111100
       0x12: 00010010
       0x11: 00010001
       0x12: 00010010
       0x7C: 01111100
       0x00: 00000000

 **/

// standard ascii 5x7 font
extern const uint8_t font5x7[256][5];