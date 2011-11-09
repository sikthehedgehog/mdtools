//***************************************************************************
// "pcm.c"
// Code for loading a raw PCM blob
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
#include <stdlib.h>
#include "main.h"

//***************************************************************************
// read_pcm
// Loads a raw PCM blob into memory
//---------------------------------------------------------------------------
// param file: pointer to file
// param where_blob: where to store pointer to blob
// param where_size: where to store blob size
// return: error code
//***************************************************************************

int read_pcm(FILE *file, uint8_t **where_blob, size_t *where_size) {
   // Set the pointer to null by default
   // We'll load the proper pointer if there are no errors
   *where_blob = NULL;

   // Attempt to get file length
   long filesize;
   if (fseek(file, 0, SEEK_END))
      return ERR_CANTREAD;
   filesize = ftell(file);
   if (filesize < 0)
      return ERR_CANTREAD;
   if (fseek(file, 0, SEEK_SET))
      return ERR_CANTREAD;

   // Allocate memory for blob
   uint8_t *blob = (uint8_t *) malloc(filesize);
   if (blob == NULL)
      return ERR_NOMEMORY;

   // Read all PCM data into RAM
   if (fread(blob, 1, filesize, file) < (size_t)(filesize)) {
      free(blob);
      return ERR_CANTREAD;
   }

   // Success!
   *where_blob = blob;
   *where_size = filesize;
   return ERR_NONE;
}
