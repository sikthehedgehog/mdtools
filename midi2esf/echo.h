//***************************************************************************
// "echo.h"
// Echo-related definitions
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

#ifndef ECHO_H
#define ECHO_H

// Some Echo limits
#define NUM_ECHOINSTR   0x100    // Number of Echo instruments

// List of Echo channels
enum {
   ECHO_FM1 = 0x00,        // FM channel #1
   ECHO_FM2 = 0x01,        // FM channel #2
   ECHO_FM3 = 0x02,        // FM channel #3
   ECHO_FM4 = 0x04,        // FM channel #4
   ECHO_FM5 = 0x05,        // FM channel #5
   ECHO_FM6 = 0x06,        // FM channel #6
   ECHO_PSG1 = 0x08,       // PSG channel #1 (square)
   ECHO_PSG2 = 0x09,       // PSG channel #2 (square)
   ECHO_PSG3 = 0x0A,       // PSG channel #3 (square)
   ECHO_PSG4 = 0x0B,       // PSG channel #4 (noise)
   ECHO_PCM = 0x0C,        // PCM channel

   NUM_ECHOCHAN = 0x10     // Number of possible Echo channels
};

// List of Echo commands
enum {
   ECHO_NOTEON = 0x00,     // Note off
   ECHO_NOTEOFF = 0x10,    // Note on
   ECHO_VOLUME = 0x20,     // Set volume
   ECHO_FREQ = 0x30,       // Set frequency
   ECHO_INSTR = 0x40,      // Set instrument
   ECHO_PAN = 0xF0,        // Set panning

   ECHO_LOOPEND = 0xFC,    // Loop end point
   ECHO_LOOPSTART = 0xFD,  // Loop start point
   ECHO_DELAY = 0xFE,      // Delay
   ECHO_STOP = 0xFF        // End of stream
};

// Function prototypes
int write_esf(const char *, int);

#endif
