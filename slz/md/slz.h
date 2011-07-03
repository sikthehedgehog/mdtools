//***************************************************************************
// "slz.h"
// Header file containing the prototype for the SLZ decompression functions
// You should include this header when using these functions
//***************************************************************************

#ifndef SLZ_H
#define SLZ_H

// Required headers
#include <stdint.h>

// Function prototypes
void decompress_slz(uint8_t *, const uint8_t *);
void decompress_slz24(uint8_t *, const uint8_t *);

#endif
