//***************************************************************************
// "pcm.c"
// Code for loading a raw PCM blob
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
