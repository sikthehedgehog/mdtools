//***************************************************************************
// "uftc.h"
// Header file containing the prototype for the UFTC decompression function
// You should include this header when using this function
//***************************************************************************

#ifndef UFTC_H
#define UFTC_H

// Required headers
#include <stdint.h>

// Function prototypes
void decompress_uftc(uint16_t *, const uint16_t *, uint16_t, uint16_t);
void decompress_uftc15(int16_t *, const int16_t *, int16_t, int16_t);

#endif
