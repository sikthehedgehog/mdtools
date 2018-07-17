//***************************************************************************
// "ewf.c"
// Code for writing an Echo waveform
//***************************************************************************
// Raw PCM to EWF conversion tool
// Copyright 2011 Javier Degirolmo
//
// This file is part of pcm2ewf.
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
