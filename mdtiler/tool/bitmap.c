//***************************************************************************
// "bitmap.c"
// Manipulating bitmaps
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
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <png.h>
#include "bitmap.h"

// Look-up table used to convert true color bitmaps into paletted ones
// Contains which color to use for each BGR combination.
#define PALTABLE_SIZE (0x20*0x20*0x20)
uint8_t pal_table[PALTABLE_SIZE];

// Prototype of callback function used by libpng
static void read_callback(png_structp, png_bytep, png_size_t);

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
// load_bitmap
// Loads a bitmap from a PNG file. Returns a pointer to the bitmap object on
// success, or NULL in case of failure.
//---------------------------------------------------------------------------
// param filename: name of file to load from
// return: pointer to bitmap or NULL on failure
//***************************************************************************

Bitmap *load_bitmap(const char *filename) {
   // Open file
   FILE *file = fopen(filename, "rb");
   if (file == NULL)
      return NULL;

   // Create PNG reading structure
   png_structp png_ptr = png_create_read_struct
      (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
   if (png_ptr == NULL) {
      fclose(file);
      return NULL;
   }

   // Create PNG info structure
   png_infop info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == NULL) {
      png_destroy_read_struct(&png_ptr, NULL, NULL);
      fclose(file);
      return NULL;
   }

   // Set up setjmp stuff which libpng wants (libpng uses setjmp to emulate
   // exception-like behavior in case of error)
   if (setjmp(png_jmpbuf(png_ptr))) {
      png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
      fclose(file);
      return NULL;
   }

   // Set up I/O functions
   png_set_read_fn(png_ptr, file, read_callback);

   // Let's not impose stupid limits... (though it may not be a good idea to
   // load such a big bitmap into RAM!)
   png_set_user_limits(png_ptr, 0x7FFFFFFF, 0x7FFFFFFF);

   // Read bitmap into memory
   png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_STRIP_16 |
      PNG_TRANSFORM_PACKING | PNG_TRANSFORM_SHIFT |
      PNG_TRANSFORM_STRIP_ALPHA, NULL);

   // Get pointers to each row
   png_bytepp rows = png_get_rows(png_ptr, info_ptr);

   // Get bitmap properties
   int32_t width = png_get_image_width(png_ptr, info_ptr);
   int32_t height = png_get_image_height(png_ptr, info_ptr);
   int type = png_get_color_type(png_ptr, info_ptr);

   // Create structure to hold the bitmap object
   Bitmap *ptr = (Bitmap *) malloc(sizeof(Bitmap));
   if (ptr == NULL) {
      png_destroy_read_struct(&png_ptr, NULL, NULL);
      fclose(file);
      return NULL;
   }
   ptr->width = width;
   ptr->height = height;
   ptr->data = NULL;
   ptr->rows = NULL;

   // Allocate enough memory to hold the pixel data
   ptr->data = (uint8_t *) malloc(width * height);
   if (ptr->data == NULL) {
      png_destroy_read_struct(&png_ptr, NULL, NULL);
      destroy_bitmap(ptr);
      fclose(file);
      return NULL;
   }

   // Allocate enough memory to hold the rows
   ptr->rows = (uint8_t **) malloc(sizeof(uint8_t *) * height);
   if (ptr->rows == NULL) {
      png_destroy_read_struct(&png_ptr, NULL, NULL);
      destroy_bitmap(ptr);
      fclose(file);
      return NULL;
   }

   // Fill up row table in the bitmap object
   for (int i = 0; i < height; i++)
      ptr->rows[i] = &ptr->data[i * width];

   // Copy data into the bitmap object
   uint8_t *dest = ptr->data;
   switch (type) {
      // Paletted
      case PNG_COLOR_TYPE_PALETTE:
         for (int y = 0; y < height; y++) {
            const uint8_t *src = rows[y];
            for (int x = 0; x < width; x++)
               *dest++ = *src++;
         }
         break;

      // Grayscale
      case PNG_COLOR_TYPE_GRAY:
         for (int y = 0; y < height; y++) {
            const uint8_t *src = rows[y];
            for (int x = 0; x < width; x++) {
               uint8_t val = *src++ >> 5;
               *dest++ = pal_table[val << 6 | val << 3 | val];
            }
         }
         break;

      // True color
      case PNG_COLOR_TYPE_RGB:
         for (int y = 0; y < height; y++) {
            const uint8_t *src = rows[y];
            for (int x = 0; x < width; x++) {
               uint16_t r = (*src++ & 0xF8) >> 3;
               uint16_t g = (*src++ & 0xF8) << 2;
               uint16_t b = (*src++ & 0xF8) << 7;
               *dest++ = pal_table[b|g|r];
            }
         }
         break;
   }

   // Success!
   png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
   fclose(file);
   return ptr;
}

//***************************************************************************
// read_callback [internal]
// Callback function used by libpng to read from the PNG file.
//---------------------------------------------------------------------------
// param png_ptr: pointer to PNG object
// param buffer: where to store the data
// param size: amount of bytes to read
//***************************************************************************

static void read_callback(png_structp png_ptr, png_bytep buffer,
png_size_t size) {
   // Attempt to read data from the file
   // In case of error let libpng know about it
   if (fread(buffer, 1, size, (FILE *) png_get_io_ptr(png_ptr)) < size)
      png_error(png_ptr, "");
}

//***************************************************************************
// get_pixel
// Retrieves the value of a pixel in a bitmap.
//---------------------------------------------------------------------------
// param ptr: pointer to bitmap
// param x: horizontal coordinate
// param y: vertical coordinate
// return: value of pixel
//***************************************************************************

uint8_t get_pixel(const Bitmap *ptr, int x, int y) {
   // Outbounds? (return a fallback value)
   if (x < 0 || y < 0 || x >= ptr->width || y >= ptr->height)
      return 0x00;

   // Return the value of the pixel at this position
   return ptr->rows[y][x];
}

//***************************************************************************
// destroy_bitmap
// Destroys a bitmap object.
//---------------------------------------------------------------------------
// param ptr: pointer to bitmap
//***************************************************************************

void destroy_bitmap(Bitmap *ptr) {
   // Deallocate memory used by the bitmap
   if (ptr->rows)
      free(ptr->rows);
   if (ptr->data)
      free(ptr->data);
   free(ptr);
}
