//***************************************************************************
// "main.h"
// Some common definitions and such
//***************************************************************************

#ifndef MAIN_H
#define MAIN_H

// Required headers
#include <stdint.h>

// Error codes
enum {
   ERR_NONE,            // No error
   ERR_CANTREAD,        // Can't read from input file
   ERR_CANTWRITE,       // Can't write into output file
   ERR_BADSIZE,         // Input file isn't suitable
   ERR_TOOSMALL,        // Input file must contain at least one tile
   ERR_TOOBIG,          // Dictionary has become too big
   ERR_CORRUPT,         // File is corrupt?
   ERR_NOMEMORY,        // Ran out of memory
   ERR_UNKNOWN          // Unknown error
};

// Function prototypes
int read_word(FILE *, uint16_t *);
int write_word(FILE *, const uint16_t);

#endif
