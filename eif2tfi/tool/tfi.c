//***************************************************************************
// "tfi.c"
// Code for storing instrument data in TFM Maker's format
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
