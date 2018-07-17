//***************************************************************************
// "tfi.c"
// Code for retrieving instrument data in TFM Maker's format
//***************************************************************************
// TFI to EIF conversion tool
// Copyright 2011 Javier Degirolmo
//
// This file is part of tfi2eif.
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

// Required headers
#include <stdint.h>
#include <stdio.h>
#include "main.h"

//***************************************************************************
// read_tfi
// Reads a FM instrument in TFM Maker's format from a file
//---------------------------------------------------------------------------
// param file: input file
// param instr: where to store instrument data
// return: error code
//***************************************************************************

int read_tfi(FILE *file, Instrument *instr) {
   // Where we'll store the file contents
   uint8_t buffer[42];

   // Read instrument data from file
   if (fread(buffer, 1, 42, file) < 42)
      return feof(file) ? ERR_CORRUPT : ERR_CANTREAD;

   // Make sure the file is 42 bytes
   // The EOF marker won't trigger until we try to read beyond the file, so
   // attempt to read another byte to ensure it's the end of the file
   if (fgetc(file) != EOF)
      return ERR_CORRUPT;

   // Retrieve global parameters
   uint8_t *ptr = buffer;
   if (*ptr > 0x07) return ERR_CORRUPT;
   instr->algorithm = *ptr++;
   if (*ptr > 0x07) return ERR_CORRUPT;
   instr->feedback = *ptr++;

   // Go through all operators
   for (int i = 0; i < 4; i++) {
      // Modulation parameters
      if (*ptr > 0x0F) return ERR_CORRUPT;
      instr->mul[i] = *ptr++;
      if (*ptr > 0x06) return ERR_CORRUPT;
      instr->dt[i] = *ptr++;

      // Total level
      if (*ptr > 0x7F) return ERR_CORRUPT;
      instr->tl[i] = *ptr++;

      // Rate parameters
      if (*ptr > 0x03) return ERR_CORRUPT;
      instr->rs[i] = *ptr++;
      if (*ptr > 0x1F) return ERR_CORRUPT;
      instr->ar[i] = *ptr++;
      if (*ptr > 0x1F) return ERR_CORRUPT;
      instr->dr[i] = *ptr++;
      if (*ptr > 0x1F) return ERR_CORRUPT;
      instr->sr[i] = *ptr++;
      if (*ptr > 0x0F) return ERR_CORRUPT;
      instr->rr[i] = *ptr++;

      // Sustain level
      if (*ptr > 0x0F) return ERR_CORRUPT;
      instr->sl[i] = *ptr++;

      // SSG-EG
      if (*ptr > 0x0F) return ERR_CORRUPT;
      instr->ssg_eg[i] = *ptr++;
   }

   // Success!
   return ERR_NONE;
}
