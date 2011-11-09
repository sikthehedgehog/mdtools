//***************************************************************************
// "main.h"
// Some common definitions and such
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

#ifndef MAIN_H
#define MAIN_H

// Error codes
enum {
   ERR_NONE,            // No error
   ERR_NOMEMORY,        // Ran out of memory
   ERR_CANTREAD,        // Can't read from input file
   ERR_CANTWRITE,       // Can't write into output file
   ERR_UNKNOWN          // Unknown error
};

#endif
