//***************************************************************************
// "eif.c"
// Code for storing instrument data in Echo's format
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
#include <stdio.h>
#include "main.h"

//***************************************************************************
// write_eif
// Writes a FM instrument in Echo's format into a file
//---------------------------------------------------------------------------
// param file: output file
// param instr: instrument to write
// return: error code
//***************************************************************************

int write_eif(FILE *file, const Instrument *instr) {
   // Look up table to convert the detune value to YM2612's format
   static uint8_t detune_table[] =
      { 0x07, 0x06, 0x05, 0x00, 0x01, 0x02, 0x03 };

   // Where we'll store the file contents
   uint8_t buffer[29];

   // Generate data for the EIF instrument
   uint8_t *ptr = buffer;
   *ptr++ = instr->algorithm | instr->feedback << 3;
   for (int i = 0; i < 4; i++)
      *ptr++ = instr->mul[i] | detune_table[instr->dt[i]] << 4;
   for (int i = 0; i < 4; i++)
      *ptr++ = instr->tl[i];
   for (int i = 0; i < 4; i++)
      *ptr++ = instr->ar[i] | instr->rs[i] << 6;
   for (int i = 0; i < 4; i++)
      *ptr++ = instr->dr[i];
   for (int i = 0; i < 4; i++)
      *ptr++ = instr->sr[i];
   for (int i = 0; i < 4; i++)
      *ptr++ = instr->rr[i] | instr->sl[i] << 4;
   for (int i = 0; i < 4; i++)
      *ptr++ = instr->ssg_eg[i];

   // Attempt to write to the file
   if (fwrite(buffer, 1, 29, file) < 29)
      return ERR_CANTWRITE;

   // Success!
   return ERR_NONE;
}
