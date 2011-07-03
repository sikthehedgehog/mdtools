//***************************************************************************
// "compress.c"
// Compresses a raw blob into an SLZ file
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
// compress
// Reads an uncompressed blob from a file and outputs SLZ compressed data
//---------------------------------------------------------------------------
// param infile: input file
// param outfile: output file
// param format: SLZ variant in use
// return: error code
//***************************************************************************

int compress(FILE *infile, FILE *outfile, int format) {
   // THIS SHOULDN'T HAPPEN
   // (also this check is to help the compiler optimize)
   if (format != FORMAT_SLZ16 && format != FORMAT_SLZ24)
      return ERR_UNKNOWN;

   // To store error codes
   int errcode;

   // Get filesize (sorry, we have to seek here!)
   if (fseek(infile, 0, SEEK_END)) return ERR_CANTREAD;
   long filesize = ftell(infile);
   if (filesize == -1) return ERR_CANTREAD;
   if (fseek(infile, 0, SEEK_SET)) return ERR_CANTREAD;

   // Empty?
   if (filesize == 0) {
      if (format == FORMAT_SLZ16)
         errcode = write_word(outfile, 0);
      else
         errcode = write_tribyte(outfile, 0);
      return errcode;
   }

   // Too large for SLZ?
   if (filesize > 0xFFFF && format == FORMAT_SLZ16)
      return ERR_TOOLARGE16;
   else if (filesize > 0xFFFFFF && format == FORMAT_SLZ24)
      return ERR_TOOLARGE24;

   // Allocate memory to store blob
   uint8_t *blob = (uint8_t *) malloc(sizeof(uint8_t) * filesize);
   if (blob == NULL) return ERR_NOMEMORY;

   // Load input file into memory
   if (fread(blob, 1, filesize, infile) < filesize) {
      free(blob);
      return ERR_CANTREAD;
   }

   // Write uncompressed size
   if (format == FORMAT_SLZ16)
      errcode = write_word(outfile, filesize);
   else
      errcode = write_tribyte(outfile, filesize);
   if (errcode) return errcode;

   // To store token data
   uint8_t tokens;
   int num_tokens = 0;

   // Buffer to store the compressed data (we need to buffer this because the
   // tokens need to be written first)
   uint8_t buffer[0x10];
   size_t bufsize = 0;

   // Scan through all bytes
   size_t pos = 0;
   while (pos < filesize) {
      // Used to store token information
      int compressed = 0;
      uint16_t dist = 3;
      uint8_t len = 2;

      // Make room for next token
      tokens <<= 1;
      num_tokens++;

      // First byte is always uncompressed
      // Also if there are too few bytes left, we can't compress them, so
      // don't bother scanning those either
      if (pos > 0 && filesize - pos >= 3) {
         // Determine maximum distance to look for
         uint16_t max_dist = pos > 0x1002 ? 0x1002 : pos;

         // Determine maximum length to check for (to avoid overflow issues)
         uint8_t max_len = filesize - pos > 18 ? 18 : filesize - pos;

         // Pointer to the strings we're going to compare
         // Making them pointers to help the compiler optimize
         const uint8_t *target = &blob[pos];
         const uint8_t *other = &blob[pos - max_dist];

         // Scan for a possible match
         size_t curr_dist;
         for (curr_dist = max_dist; curr_dist >= 3; curr_dist--, other++)
         {
            // To avoid wasting time on matches that WILL fail...
            if (*other != *target)
               continue;

            // Check all lengths that are larger than the longest string we
            // found so far (don't bother with shorter strings as that'd be
            // a waste)
            uint8_t curr_len;
            for (curr_len = max_len; curr_len > len; curr_len--) {
               // Did we find a match? (if so, don't bother with smaller
               // string, also mark the token as compressable)
               if (!memcmp(other, target, curr_len)) {
                  dist = curr_dist;
                  len = curr_len;
                  compressed = 1;
                  break;
               }
            }
         }
      }

      // Compressed token?
      if (compressed) {
         // Skip repeated string in the blob
         pos += len;

         // Store contents of the token
         dist -= 3;
         len -= 3;
         buffer[bufsize] = dist >> 4;
         buffer[bufsize+1] = dist << 4 | len;
         bufsize += 2;

         // Mark token as compressed
         tokens |= 1;
      }

      // Uncompressed token?
      else {
         buffer[bufsize] = blob[pos];
         pos++;
         bufsize++;
      }

      // Huh, done with this group of tokens?
      if (num_tokens == 8) {
         // Write token types
         if (fwrite(&tokens, 1, 1, outfile) < 1) {
            free(blob);
            return ERR_CANTWRITE;
         }

         // Write buffered data
         if (fwrite(buffer, 1, bufsize, outfile) < bufsize) {
            free(blob);
            return ERR_CANTWRITE;
         }

         // Reset tokens and buffer
         tokens = 0;
         num_tokens = 0;
         bufsize = 0;
      }
   }

   // We don't need the blob anymore
   free(blob);

   // If there are any tokens left, make sure they're written too
   if (num_tokens < 8) {
      // Make sure token types are MSB aligned
      tokens <<= 8 - num_tokens;

      // Write token types
      if (fwrite(&tokens, 1, 1, outfile) < 1)
         return ERR_CANTWRITE;

      // Write buffered data
      if (fwrite(buffer, 1, bufsize, outfile) < bufsize)
         return ERR_CANTWRITE;
   }

   // Success!
   return ERR_NONE;
}
