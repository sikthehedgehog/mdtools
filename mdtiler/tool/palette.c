//***************************************************************************
// "palette.c"
// Handles palette remapping functionality
//***************************************************************************
// mdtiler - Bitmap to tile conversion tool
// Copyright 2018 Javier Degirolmo
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

#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <stdlib.h>
#include "palette.h"

// Look-up table used to convert true color bitmaps into paletted ones
// Contains which color to use for each BGR combination.
#define PALTABLE_SIZE (0x20*0x20*0x20)
uint8_t pal_table[PALTABLE_SIZE];

// Row-to-palette mappings
static unsigned mappings[0x10] = {
    0, 1, 2, 3, 4, 5, 6, 7,
    0, 1, 2, 3, 4, 5, 6, 7
};

//***************************************************************************
// set_palette
// Sets the palette to use to convert true color bitmaps into paletted ones.
// This function takes an array of 16 colors, each color being a MD color
// (i.e. BXGXRX 3.1.3.1.3.1)
//---------------------------------------------------------------------------
// param colors: pointer to palette (16 entries)
//***************************************************************************

void set_palette(const uint16_t *colors) {
   // Go through the entire palette
   for (uint16_t i = 0; i < PALTABLE_SIZE; i++) {
      // Get BGR components for the color to check
      uint8_t sb = i >> 10 & 0x1F;
      uint8_t sg = i >> 5 & 0x1F;
      uint8_t sr = i & 0x1F;

      // Used to determine which is the best match
      uint8_t best = 0;
      unsigned min_diff = UINT_MAX;

      // Check it against every color in the palette
      for (uint8_t i = 0; i < 0x10; i++) {
         // Get BGR components for this palette entry
         uint8_t db = colors[i] >> 9 & 0x07;
         uint8_t dg = colors[i] >> 5 & 0x07;
         uint8_t dr = colors[i] >> 1 & 0x07;
         db = db << 2 | db >> 3;
         dg = dg << 2 | dg >> 3;
         dr = dr << 2 | dr >> 3;

         // Get how different are the colors
         unsigned diff = abs(dr - sr) + abs(dg - sg) + abs(db - sb);

         // Is this color a better match?
         if (diff <= min_diff) {
            best = i;
            min_diff = diff;
            if (diff == 0)
               break;
         }
      }

      // Store best match into the look-up table
      pal_table[i] = best;
   }
}

//***************************************************************************
// get_palette_mapping
// Gets the palette mapping corresponding to a given color group
//---------------------------------------------------------------------------
// param group: 16-color group to get palette for
// return: palette ID
//***************************************************************************

unsigned get_palette_mapping(unsigned group) {
    return mappings[group];
}

//***************************************************************************
// remap_palette
// Remaps a color group to another palette
//---------------------------------------------------------------------------
// param group: 16-color group to affect
// param which: new palette to remap to
//***************************************************************************

void remap_palette(unsigned group, unsigned which) {
    mappings[group & 0x0F] = which & 0x07;
}
