//***************************************************************************
// "tiles.c"
// Tile fetching stuff
//***************************************************************************
// mdtiler - Bitmap to tile conversion tool
// Copyright 2011, 2012 Javier Degirolmo
//
// This file is part of mdtiler.
//
// mdtiler is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// mdtiler is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with mdtiler.  If not, see <http://www.gnu.org/licenses/>.
//***************************************************************************

// Required headers
#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "bitmap.h"
#include "tiles.h"

// Prototype for functions used to fetch tiles
typedef int TileFunc(const Bitmap *, FILE *, int, int);

// Current tile output format
static Format format = FORMAT_4BPP;

//***************************************************************************
// set_output_format
// Changes the output format for tiles
//---------------------------------------------------------------------------
// param value: new format
//***************************************************************************

void set_output_format(Format value) {
   format = value;
}

//***************************************************************************
// fetch_tile_1bpp
// Reads a tile from the bitmap and outputs a 1bpp tile
//---------------------------------------------------------------------------
// param in: input bitmap
// param out: output file
// param xr: base X coordinate (rightmost pixel of tile)
// param yr: base Y coordinate (topmost pixel of tile)
// return: error code
//***************************************************************************

static int fetch_tile_1bpp(const Bitmap *in, FILE *out, int bx, int by)
{
   // To store the tile data
   uint8_t data[8];

   // Read the tile from the bitmap to generate the 4bpp data
   uint8_t *ptr = data;
   for (int y = 0; y < 8; y++, ptr++) {
      uint8_t temp = 0;
      for (int x = 0; x < 8; x++) {
         temp <<= 1;
         temp |= get_pixel(in, bx + x, by + y) & 0x01;
      }
      *ptr = temp;
   }

   // Write tile blob into output file
   if (fwrite(data, 1, 8, out) < 8)
      return ERR_CANTWRITE;

   // Success!
   return ERR_NONE;
}

//***************************************************************************
// fetch_tile_4bpp
// Reads a tile from the bitmap and outputs a 4bpp tile
//---------------------------------------------------------------------------
// param in: input bitmap
// param out: output file
// param xr: base X coordinate (rightmost pixel of tile)
// param yr: base Y coordinate (topmost pixel of tile)
// return: error code
//***************************************************************************

static int fetch_tile_4bpp(const Bitmap *in, FILE *out, int bx, int by)
{
   // To store the tile data
   uint8_t data[32];

   // Read the tile from the bitmap to generate the 4bpp data
   uint8_t *ptr = data;
   for (int y = 0; y < 8; y++)
   for (int x = 0; x < 8; x += 2) {
      *ptr++ = (get_pixel(in, bx + x, by + y) << 4) |
               (get_pixel(in, bx + x + 1, by + y) & 0x0F);
   }

   // Write tile blob into output file
   if (fwrite(data, 1, 32, out) < 32)
      return ERR_CANTWRITE;

   // Success!
   return ERR_NONE;
}

//***************************************************************************
// fetch_tile_error
// Tile fetching function to use if there was an error with the format
//---------------------------------------------------------------------------
// param in: (ignored)
// param out: (ignored)
// param xr: (ignored)
// param yr: (ignored)
// return: ERR_UNKNOWN
//***************************************************************************

static int fetch_tile_error(const Bitmap *in, FILE *out, int bx, int by)
{
   // To shut up the compiler
   (void) in;
   (void) out;
   (void) bx;
   (void) by;

   // Don't do anything, just panic
   return ERR_UNKNOWN;
}

//***************************************************************************
// get_tile_func
// Returns which function to use to read tiles in the current format
//---------------------------------------------------------------------------
// return: pointer to function
//***************************************************************************

static inline TileFunc *get_tile_func(void) {
   // Return the adequate function for this format
   // fetch_tile_error is used if there's a bug in the program...
   switch (format) {
      case FORMAT_4BPP: return fetch_tile_4bpp;
      case FORMAT_1BPP: return fetch_tile_1bpp;
      default: return fetch_tile_error;
   }
}

//***************************************************************************
// write_tilemap
// Outputs a block of tiles using tilemap ordering
//---------------------------------------------------------------------------
// param in: input bitmap
// param out: output file
// param xr: base X coordinate (rightmost pixel of tile)
// param yr: base Y coordinate (topmost pixel of tile)
// param width: width in tiles
// param height: height in tiles
// return: error code
//***************************************************************************

int write_tilemap(const Bitmap *in, FILE *out, int bx, int by,
int width, int height) {
   // Determine function we're going to use to fetch tiles
   TileFunc *func = get_tile_func();

   // Traverse through all tiles in tilemap ordering
   // (left-to-right, then top-to-bottom)
   for (int y = 0; y < height; y++)
   for (int x = 0; x < width; x++) {
      int errcode = func(in, out, bx + (x << 3), by + (y << 3));
      if (errcode) return errcode;
   }

   // Success!
   return ERR_NONE;
}

//***************************************************************************
// write_sprite
// Outputs a block of tiles using sprite ordering
//---------------------------------------------------------------------------
// param in: input bitmap
// param out: output file
// param xr: base X coordinate (rightmost pixel of tile)
// param yr: base Y coordinate (topmost pixel of tile)
// param width: width in tiles
// param height: height in tiles
// return: error code
//***************************************************************************

int write_sprite(const Bitmap *in, FILE *out, int bx, int by,
int width, int height) {
   // Determine function we're going to use to fetch tiles
   TileFunc *func = get_tile_func();

   // Sprites are at most 4 tiles high, so split sprites into strips that
   // have at most that length
   while (height > 0) {
      // Determine height for this strip
      int strip_height = height > 4 ? 4 : height;

      // Traverse through all tiles in sprite ordering
      // (top-to-bottom, then left-to-right)
      for (int x = 0; x < width; x++)
      for (int y = 0; y < strip_height; y++) {
         int errcode = func(in, out, bx + (x << 3), by + (y << 3));
         if (errcode) return errcode;
      }

      // Move onto the next strip
      height -= strip_height;
      by += strip_height << 3;
   }

   // Success!
   return ERR_NONE;
}
