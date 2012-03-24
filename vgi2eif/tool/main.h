//***************************************************************************
// "main.h"
// Some common definitions and such
//***************************************************************************
// VGI to EIF conversion tool
// Copyright 2012 Javier Degirolmo
//
// This file is part of vgi2eif.
//
// vgi2eif is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// vgi2eif is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with vgi2eif.  If not, see <http://www.gnu.org/licenses/>.
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
   ERR_CORRUPT,         // Input file is corrupt?
   ERR_UNKNOWN          // Unknown error
};

// Data for a FM instrument
typedef struct {
   // Global parameters
   uint8_t algorithm;      // Algorithm (00..07)
   uint8_t feedback;       // Feedback (00..07)

   // Operator parameters
   uint8_t mul[4];         // Multiplier (00..0F)
   uint8_t dt[4];          // Detune (00..06)
   uint8_t tl[4];          // Total level (00..7F)
   uint8_t rs[4];          // Rate scaling (00..03)
   uint8_t ar[4];          // Attack rate (00..1F)
   uint8_t dr[4];          // Decay rate (00..1F), MSB for AM enable
   uint8_t sr[4];          // Sustain rate (00..1F)
   uint8_t rr[4];          // Release rate (00..0F)
   uint8_t sl[4];          // Sustain level (00..0F)
   uint8_t ssg_eg[4];      // SSG-EG envelope (00..0F)
} Instrument;

#endif
