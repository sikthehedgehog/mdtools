//***************************************************************************
// "main.h"
// Some common definitions and such
//***************************************************************************
// Mega Drive header generation tool
// Copyright 2011 Javier Degirolmo
//
// This file is part of headgen.
//
// headgen is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// headgen is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with headgen.  If not, see <http://www.gnu.org/licenses/>.
//***************************************************************************

#ifndef MAIN_H
#define MAIN_H

// Error codes
enum {
   ERR_NONE,            // No error
   ERR_CANTWRITE,       // Can't write into output file
   ERR_UNKNOWN          // Unknown error
};

// Some limits (defined by the header format, mind you)
#define MAX_TITLE       48    // Max length of game title
#define MAX_COPYRIGHT   4     // Max length of copyright code
#define MAX_DEVICES     16    // Max length of device support

// To hold the information to show in the header
typedef struct {
   char title[MAX_TITLE+1];               // Game title
   char copyright[MAX_COPYRIGHT+1];       // Copyright code
   unsigned year;                         // Release year
   unsigned month;                        // Release month

   unsigned pad6: 1;                      // Set if 6-pad is supported
   unsigned mouse: 1;                     // Set if mouse is supported
   unsigned megacd: 1;                    // Set if Mega CD is supported
   unsigned sram: 1;                      // Set if SRAM is supported
} HeaderInfo;

#endif
