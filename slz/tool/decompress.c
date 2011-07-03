//***************************************************************************
// "decompress.c"
// Decompresses a SLZ file into a raw blob
//***************************************************************************
// Slz compression tool
// Copyright 2011 Javier Degirolmo
//
// This file is part of the slz tool.
//
// The slz tool is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// The slz tool is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with the slz tool.  If not, see <http://www.gnu.org/licenses/>.
//***************************************************************************

// Required headers
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"

//***************************************************************************
// decompress
// Reads a file in SLZ format and outputs an uncompressed blob
//---------------------------------------------------------------------------
// param infile: input file
// param outfile: output file
// param format: SLZ variant in use
// return: error code
//***************************************************************************

int decompress(FILE *infile, FILE *outfile, int format) {
   // THIS SHOULDN'T HAPPEN
   // (also this check is to help the compiler optimize)
   if (format != FORMAT_SLZ16 && format != FORMAT_SLZ24)
      return ERR_UNKNOWN;

   // To store error codes
   int errcode;

   // Read uncompressed size
   uint32_t size;
   if (format == FORMAT_SLZ16) {
      uint16_t temp;
      errcode = read_word(infile, &temp);
      size = temp;
   } else
      errcode = read_tribyte(infile, &size);
   if (errcode) return errcode;

   // No data? If so, we're done!
   if (size == 0)
      return ERR_NONE;

   // Allocate memory to store the data
   uint8_t *data = (uint8_t *) malloc(sizeof(uint8_t) * size);
   if (data == NULL)
      return ERR_NOMEMORY;

   // To store token data
   uint8_t tokens;
   uint8_t num_tokens = 0;

   // Decompress all data
   size_t pos = 0;
   while (pos < size) {
      // Need more tokens?
      if (num_tokens == 0) {
         num_tokens = 8;
         if (fread(&tokens, 1, 1, infile) < 1) {
            free(data);
            return ferror(infile) ? ERR_CANTREAD : ERR_CORRUPT;
         }
      }

      // Compressed string?
      if (tokens & 0x80) {
         // Get string information
         uint16_t info;
         errcode = read_word(infile, &info);
         if (errcode) { free(data); return errcode; }

         // Get distance and length
         uint16_t dist = (info >> 4) + 3;
         uint8_t len = (info & 0x0F) + 3;

         // Er, make sure parameters are valid...
         if (dist > pos) {
            free(data);
            return ERR_CORRUPT;
         }

         // Copy string!
         uint8_t *dest = data + pos;
         uint8_t *src = dest - dist;
         pos += len;
         while (len-- > 0)
            *dest++ = *src++;
      }

      // Uncompressed byte?
      else {
         if (fread(&data[pos], 1, 1, infile) < 1) {
            free(data);
            return ferror(infile) ? ERR_CANTREAD : ERR_CORRUPT;
         }
         pos++;
      }

      // Go for next token
      tokens <<= 1;
      num_tokens--;
   }

   // Not the expected size?
   if (pos != size) {
      free(data);
      return ERR_CORRUPT;
   }

   // Write decompressed data into output file
   if (fwrite(data, 1, size, outfile) < size) {
      free(data);
      return ERR_CANTWRITE;
   }

   // Success!
   free(data);
   return ERR_NONE;
}
