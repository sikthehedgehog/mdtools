//***************************************************************************
// "tfi.c"
// Code for storing instrument data in TFM Maker's format
//***************************************************************************
// EIF to TFI conversion tool
// Copyright 2015 Javier Degirolmo
//
// This file is part of eif2tfi.
//
// eif2tfi is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// eif2tfi is distributed in the hope that it will be useful,
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
// write_tfi
// Writes a FM instrument in TFM Maker's format into a file
//---------------------------------------------------------------------------
// param file: output file
// param instr: instrument to write
// return: error code
//***************************************************************************

int write_tfi(FILE *file, const Instrument *instr) {
   // Where we'll store the file contents
   uint8_t buffer[42];

   // Store global parameters
   uint8_t *ptr = buffer;
   *ptr++ = instr->algorithm;
   *ptr++ = instr->feedback;

   // Store each operator
   for (int i = 0; i < 4; i++) {
      *ptr++ = instr->mul[i];
      *ptr++ = instr->dt[i];
      *ptr++ = instr->tl[i];
      *ptr++ = instr->rs[i];
      *ptr++ = instr->ar[i];
      *ptr++ = instr->dr[i];
      *ptr++ = instr->sr[i];
      *ptr++ = instr->rr[i];
      *ptr++ = instr->sl[i];
      *ptr++ = instr->ssg_eg[i];
   }

   // Attempt to write to the file
   if (fwrite(buffer, 1, 42, file) < 42)
      return ERR_CANTWRITE;

   // Success!
   return ERR_NONE;
}
