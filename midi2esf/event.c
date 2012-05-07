//***************************************************************************
// "event.c"
// Music event generation and handling
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

// Required headers
#include <stddef.h>
#include <stdlib.h>
#include "echo.h"
#include "event.h"

// Where events are stored
static Event *list = NULL;

//***************************************************************************
// add_event
// Creates a new event and returns it.
//***************************************************************************

Event *add_event(uint64_t timestamp) {
   // Make room for a new event in the list
   Event *temp = (Event *) malloc(sizeof(Event));
   if (temp == NULL) return NULL;

   // Store timestamp
   temp->timestamp = timestamp;

   // First event?
   if (list == NULL) {
      list = temp;
      temp->prev = NULL;
      temp->next = NULL;
   }

   // Nope, find out where to put the event based on the timestamp
   else for (Event *e = list; ; e = e->next) {
      if (e->timestamp > timestamp) {
         e->prev->next = temp;
         temp->prev = e->prev;
         temp->next = e;
         e->prev = temp;
         break;
      }
      if (e->next == NULL) {
         temp->next = NULL;
         temp->prev = e;
         e->next = temp;
         break;
      }
   }

   // Set some sensible defaults
   temp->channel = CHAN_NONE;
   temp->param = 0;
   temp->instrument = -1;
   temp->volume = -1;
   temp->panning = -1;

   // Return a pointer to the last event just added
   return temp;
}

//***************************************************************************
// get_events
// Returns the event list so other code can examine it. The pointer is const,
// so the list still can't be altered. The pointer is rendered void if the
// list is expanded or reset.
//---------------------------------------------------------------------------
// return: pointer to event list
//***************************************************************************

const Event *get_events(void) {
   return list;
}

//***************************************************************************
// reset_events
// Resets the event list (leaving it clear).
//***************************************************************************

void reset_events(void) {
   // Deallocate all events
   for (Event *event = list; event != NULL;) {
      Event *next = event->next;
      free(event);
      event = next;
   }
   list = NULL;
}
