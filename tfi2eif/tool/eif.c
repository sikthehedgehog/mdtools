//***************************************************************************
// "eif.c"
// Code for storing instrument data in Echo's format
//***************************************************************************
// TFI to EIF conversion tool
// Copyright 2011 Javier Degirolmo
//
// This file is part of tfi2eif.
//
// tfi2eif is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// tfi2eif is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with tfi2eif.  If not, see <http://www.gnu.org/licenses/>.
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
