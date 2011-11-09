//***************************************************************************
// "ewf.c"
// Code for writing an Echo waveform
//***************************************************************************
// Raw PCM to EWF conversion tool
// Copyright 2011 Javier Degirolmo
//
// This file is part of pcm2ewf.
//
// pcm2ewf is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// pcm2ewf is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with pcm2ewf.  If not, see <http://www.gnu.org/licenses/>.
//***************************************************************************

// Required headers
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include "main.h"

//***************************************************************************
// write_ewf
// Parses PCM data and writes an EWF file
//---------------------------------------------------------------------------
// param file: pointer to file
// param blob: pointer to PCM data
// param size: size of PCM data
// return: error code
//***************************************************************************

int write_ewf(FILE *file, uint8_t *blob, size_t size) {
   // Go through the entire blob and filter out 0xFF samples, since those
   // aren't allowed in EWF (a byte with value 0xFF would mark the end of the
   // waveform).
   uint8_t *ptr = blob;
   for (size_t i = 0; i < size; i++, ptr++)
      if (*ptr == 0xFF) *ptr = 0xFE;

   // Write filtered blob into EWF file
   if (fwrite(blob, 1, size, file) < size)
      return ERR_CANTWRITE;

   // Write end-of-file marker for EWF
   if (fputc(0xFF, file) == EOF)
      return ERR_CANTWRITE;

   // Success!
   return ERR_NONE;
}
