//***************************************************************************
// "tiles.h"
// Header file for "tiles.c"
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

#ifndef TILES_H
#define TILES_H

// Required headers
#include <stdio.h>
#include <picel.h>

// Possible tile output formats
typedef enum {
   FORMAT_DEFAULT,      // Use default format
   FORMAT_4BPP,         // 4bpp tiles
   FORMAT_1BPP,         // 1bpp tiles
   FORMAT_TOOMANY       // Too many formats specified
} Format;

// Function prototypes
void set_output_format(Format);
int write_tilemap(const PicelBitmap *, FILE *, int, int, int, int);
int write_sprite(const PicelBitmap *, FILE *, int, int, int, int);

#endif
