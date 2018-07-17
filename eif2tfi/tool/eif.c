//***************************************************************************
// "eif.c"
// Code for retrieving instrument data in Echo's format
//***************************************************************************
// EIF to TFI conversion tool
// Copyright 2015 Javier Degirolmo
//
// This file is part of eif2tfi.
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
// read_eif
// Reads a FM instrument in Echo's format from a file
//---------------------------------------------------------------------------
// param file: input file
// param instr: where to store instrument data
// return: error code
//***************************************************************************

int read_eif(FILE *file, Instrument *instr) {
   // Look up table to convert the detune value from YM2612's format
   static const uint8_t detune_table[] =
      { 3, 4, 5, 6, 3, 2, 1, 0 };

   // Where we'll store the file contents
   uint8_t buffer[29];

   // Read instrument data from file
   if (fread(buffer, 1, 29, file) < 29)
      return feof(file) ? ERR_CORRUPT : ERR_CANTREAD;

   // Make sure the file is 29 bytes
   // The EOF marker won't trigger until we try to read beyond the file, so
   // attempt to read another byte to ensure it's the end of the file
   if (fgetc(file) != EOF)
      return ERR_CORRUPT;

   // Check that all values are valid
   if (buffer[0] & 0xC0)
      return ERR_CORRUPT;
   for (int i = 0; i < 4; i++) {
      if (buffer[0x01 + i] & 0x80) return ERR_CORRUPT;
      if (buffer[0x05 + i] & 0x80) return ERR_CORRUPT;
      if (buffer[0x09 + i] & 0x20) return ERR_CORRUPT;
      if (buffer[0x0D + i] & 0xE0) return ERR_CORRUPT;
      if (buffer[0x11 + i] & 0xE0) return ERR_CORRUPT;
      if (buffer[0x19 + i] & 0xF0) return ERR_CORRUPT;
   }

   // Retrieve global parameters
   instr->algorithm = buffer[0] & 0x07;
   instr->feedback = buffer[0] >> 3;

   // Retrieve operator-specific parameters
   for (int i = 0; i < 4; i++) {
      instr->mul[i] = buffer[0x01 + i] & 0x0F;
      instr->dt[i] = detune_table[buffer[0x01 + i] >> 4 & 0x03];
      instr->tl[i] = buffer[0x05 + i] & 0x7F;
      instr->rs[i] = buffer[0x09 + i] >> 6;
      instr->ar[i] = buffer[0x09 + i] & 0x1F;
      instr->dr[i] = buffer[0x0D + i] & 0x1F;
      instr->sr[i] = buffer[0x11 + i] & 0x1F;
      instr->rr[i] = buffer[0x15 + i] & 0x0F;
      instr->sl[i] = buffer[0x15 + i] >> 4;
      instr->ssg_eg[i] = buffer[0x19 + i] & 0x0F;
   }

   // Success!
   return ERR_NONE;
}
