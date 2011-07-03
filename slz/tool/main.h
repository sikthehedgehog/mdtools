//***************************************************************************
// "main.h"
// Some common definitions and such
//***************************************************************************
// Slz compression tool
// Copyright 2011 Javier Degirolmo
//
// This file is part of the slz tool.
//
// The slz tool is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The slz tool is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with the slz tool.  If not, see <http://www.gnu.org/licenses/>.
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
   ERR_TOOLARGE16,      // File is too large for SLZ16
   ERR_TOOLARGE24,      // File is too large for SLZ24
   ERR_CORRUPT,         // File is corrupt?
   ERR_NOMEMORY,        // Ran out of memory
   ERR_UNKNOWN          // Unknown error
};

// Possible formats
enum {
   FORMAT_DEFAULT,      // No format specified
   FORMAT_SLZ16,        // SLZ16 (16-bit size)
   FORMAT_SLZ24,        // SLZ24 (24-bit size)
   FORMAT_TOOMANY       // Too many formats specified
};

// Function prototypes
int read_word(FILE *, uint16_t *);
int read_tribyte(FILE *, uint32_t *);
int write_word(FILE *, const uint16_t);
int write_tribyte(FILE *, const uint32_t);

#endif
