//***************************************************************************
// "midi.h"
// MIDI-related definitions
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

#ifndef MIDI_H
#define MIDI_H

// Some MIDI limits
#define NUM_MIDICHAN       0x10     // Number of MIDI channels
#define NUM_MIDIINSTR      0x80     // Number of MIDI instruments

// Possible types of instrument mappings
typedef enum {
   INSTR_FM,               // FM instrument
   INSTR_PSG,              // PSG instrument
   INSTR_PCM,              // PCM instrument
   NUM_INSTRTYPES          // Number of instrument types
} InstrType;

// Function prototypes
int read_midi(const char *);
void map_channel(int, int);
void map_instrument(int, int, int, int, int);
void set_pitch_range(int);

#endif
