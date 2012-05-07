//***************************************************************************
// "main.h"
// Some common definitions and such
//***************************************************************************
// MIDI to ESF conversion tool
// Copyright 2012 Javier Degirolmo
//
// This file is part of midi2esf.
//
// midi2esf is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// midi2esf is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with midi2esf.  If not, see <http://www.gnu.org/licenses/>.
//***************************************************************************

#ifndef MAIN_H
#define MAIN_H

// Error codes
enum {
   ERR_NONE,            // No error
   ERR_OPENBATCH,       // Can't open batch file
   ERR_READBATCH,       // Can't read from batch file
   ERR_OPENMIDI,        // Can't open input MIDI file
   ERR_READMIDI,        // Can't read input MIDI file
   ERR_CORRUPT,         // Corrupt MIDI file
   ERR_MIDITYPE2,       // MIDI type 2 (not supported)
   ERR_OPENESF,         // Can't open output ESF file
   ERR_WRITEESF,        // Can't write to ESF file
   ERR_NOMEMORY,        // Ran out of memory
   ERR_BADQUOTE,        // Quote inside non-quoted token
   ERR_NOQUOTE,         // Missing ending quote
   ERR_PARSE,           // Batch file has parsing errors
   ERR_UNKNOWN          // Unknown error
};

#endif
