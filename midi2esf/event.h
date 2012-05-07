//***************************************************************************
// "event.h"
// Event-related definitions
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

#ifndef EVENT_H
#define EVENT_H

// Required headers
#include <stddef.h>
#include <stdint.h>

// List of logical channels (NOT Echo channels)
// Events are mapped to one of these channels
enum {
   CHAN_FM1,               // FM channel 1
   CHAN_FM2,               // FM channel 2
   CHAN_FM3,               // FM channel 3
   CHAN_FM4,               // FM channel 4
   CHAN_FM5,               // FM channel 5
   CHAN_FM6,               // FM channel 6
   CHAN_PSG1,              // PSG channel 1
   CHAN_PSG2,              // PSG channel 2
   CHAN_PSG3,              // PSG channel 3
   CHAN_PSG4,              // PSG channel 4
   CHAN_PSG4EX,            // PSG channels 3+4
   CHAN_PCM,               // PCM channel

   NUM_CHAN,               // Number of logical channels
   CHAN_NONE               // No channel assigned
};

// Possible types of event
enum {
   EVENT_NOTEON,           // Note on
   EVENT_NOTEOFF,          // Note off
   EVENT_SLIDE,            // Change pitch
   EVENT_VOLUME,           // Change volume
   EVENT_PAN               // Change panning
};

// Information for an event
// These are *not* ESF events, but rather music events we generate when
// parsing the MIDI stream. Then the ESF parser will read these events and
// generate proper ESF events in the output file.
typedef struct Event {
   uint64_t timestamp;     // Timestamp (in frames, 48.16 fixed comma)
   struct Event *next;     // Pointer to next event
   struct Event *prev;     // Pointer to previous event

   int16_t param;          // Event parameter
   uint8_t type;           // Type of event
   uint8_t channel;        // Target channel
   int16_t instrument;     // Echo instrument (-1 if not specified)
   int16_t volume;         // Echo volume (-1 if not specified)
   int16_t panning;        // Echo panning (-1 if not specified)
} Event;

// Function prototypes
Event *add_event(uint64_t);
const Event *get_events(void);
void reset_events(void);

#endif
