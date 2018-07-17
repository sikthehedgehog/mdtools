//***************************************************************************
// "main.h"
// Some common definitions and such
//***************************************************************************
// VGI to EIF conversion tool
// Copyright 2012 Javier Degirolmo
//
// This file is part of vgi2eif.
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
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
