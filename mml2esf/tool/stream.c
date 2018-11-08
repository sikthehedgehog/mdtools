// Required headers
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "stream.h"

// List of events
static Event *events = NULL;
static size_t num_events = 0;

// Private function prototypes
static void alloc_event(uint64_t, unsigned, EventType, unsigned);
static int sort_func(const void *, const void *);

//***************************************************************************
// alloc_event [internal]
// Allocates a new event into the stream.
//---------------------------------------------------------------------------
// param timestamp: event timestamp
// param channel: affected channel
// param type: what kind of event is it
// param value: argument for event
//***************************************************************************

static void alloc_event(uint64_t timestamp, unsigned channel,
EventType type, unsigned value) {
   // Allocate room for new event
   num_events++;
   Event *tmp = (Event*) realloc(events, sizeof(Event) * num_events);
   if (tmp == NULL) {
      fprintf(stderr, "Error: out of memory!\n");
      exit(EXIT_FAILURE);
   } else {
      events = tmp;
   }

   // Fill in new event
   Event *ev = &events[num_events - 1];
   ev->timestamp = timestamp;
   ev->channel = channel;
   ev->type = type;
   ev->value = value;
}

//***************************************************************************
// add_nop
// Adds a no-op event to the stream.
//---------------------------------------------------------------------------
// param timestamp: event timestamp
//***************************************************************************

void add_nop(uint64_t timestamp)
{
   alloc_event(timestamp, 0, EV_NOP, 0);
}

//***************************************************************************
// add_note_on
// Adds a note on event to the stream.
//---------------------------------------------------------------------------
// param timestamp: event timestamp
// param channel: affected channel
// param note: note to play
//***************************************************************************

void add_note_on(uint64_t timestamp, unsigned channel, unsigned note)
{
   alloc_event(timestamp, channel, EV_NOTEON, note);
}

//***************************************************************************
// add_note_off
// Adds a note off event to the stream.
//---------------------------------------------------------------------------
// param timestamp: event timestamp
// param channel: affected channel
//***************************************************************************

void add_note_off(uint64_t timestamp, unsigned channel)
{
   alloc_event(timestamp, channel, EV_NOTEOFF, 0);
}

//***************************************************************************
// add_set_note
// Adds a set semitone event to the stream.
//---------------------------------------------------------------------------
// param timestamp: event timestamp
// param channel: affected channel
// param semitone: new semitone
//***************************************************************************

void add_set_note(uint64_t timestamp, unsigned channel,
unsigned semitone)
{
   alloc_event(timestamp, channel, EV_SETNOTE, semitone);
}

//***************************************************************************
// add_set_freq
// Adds a set frequency event to the stream.
//---------------------------------------------------------------------------
// param timestamp: event timestamp
// param channel: affected channel
// param freq: new frequency
// param octave: new octave
//***************************************************************************

void add_set_freq(uint64_t timestamp, unsigned channel,
unsigned freq, unsigned octave)
{
   // Process the frequency and octave into the format Echo wants
   // The exact format depends on the kind of channel
   uint16_t raw;
   if (/*channel >= 0x00 && */ channel <= 0x07) {
      raw = freq | (octave << 11);
   } else if (channel >= 0x08 && channel <= 0x0A) {
      raw = freq >> octave;
      raw = ((raw & 0x0F) << 8) | (raw >> 4);
   } else {
      raw = freq;
   }

   // Now issue the event
   alloc_event(timestamp, channel, EV_SETFREQ, raw);
}

//***************************************************************************
// add_set_vol
// Adds a set volume to the stream.
//---------------------------------------------------------------------------
// param timestamp: event timestamp
// param channel: affected channel
// param volume: new volume
//***************************************************************************

void add_set_vol(uint64_t timestamp, unsigned channel, unsigned volume)
{
   // Volume is actually attenuation
   volume = 0x0F - volume;

   // FM uses a different volume range
   // Don't make it linear though to get more useful values
   if (/*channel >= 0x00 && */ channel <= 0x07) {
      static unsigned table[] = {
         0x00, 0x02, 0x04, 0x06,
         0x08, 0x0A, 0x0C, 0x0E,
         0x10, 0x14, 0x18, 0x1C,
         0x20, 0x30, 0x40, 0x7F
      };
      volume = table[volume];
   }

   // Now issue the event
   alloc_event(timestamp, channel, EV_SETVOL, volume);
}

//***************************************************************************
// add_set_pan
// Adds a set panning event to the stream.
//---------------------------------------------------------------------------
// param timestamp: event timestamp
// param channel: affected channel
// param instrument: new instrument
//***************************************************************************

void add_set_pan(uint64_t timestamp, unsigned channel, unsigned panning)
{
   alloc_event(timestamp, channel, EV_SETPAN, panning);
}

//***************************************************************************
// add_set_instr
// Adds a set instrument event to the stream.
//---------------------------------------------------------------------------
// param timestamp: event timestamp
// param channel: affected channel
// param instrument: new instrument
//***************************************************************************

void add_set_instr(uint64_t timestamp, unsigned channel, unsigned instrument)
{
   alloc_event(timestamp, channel, EV_SETINSTR, instrument);
}

//***************************************************************************
// add_set_tempo
// Adds a set tempo event to the stream.
//---------------------------------------------------------------------------
// param timestamp: event timestamp
// param tempo: new tempo for future events
//***************************************************************************

void add_set_tempo(uint64_t timestamp, unsigned tempo)
{
   alloc_event(timestamp, 0, EV_SETTEMPO, tempo);
}

//***************************************************************************
// add_set_reg
// Adds a set register event to the stream.
//---------------------------------------------------------------------------
// param timestamp: event timestamp
// param register: affected register
// param value: new value
//***************************************************************************

void add_set_reg(uint64_t timestamp, unsigned reg, unsigned value)
{
   alloc_event(timestamp, reg, EV_SETREG, value);
}

//***************************************************************************
// add_set_flags
// Adds a set/clear flags event to the stream.
//---------------------------------------------------------------------------
// param timestamp: event timestamp
// param setclr: 1 = set, 0 = clear
// param flags: flags bitfield
//***************************************************************************

void add_set_flags(uint64_t timestamp, unsigned setclr, unsigned flags)
{
   alloc_event(timestamp, setclr, EV_FLAGS, flags);
}

//***************************************************************************
// add_lock
// Adds a lock channel event to the stream.
//---------------------------------------------------------------------------
// param timestamp: event timestamp
// param channel: affected channel
//***************************************************************************

void add_lock(uint64_t timestamp, unsigned channel)
{
   alloc_event(timestamp, channel, EV_LOCK, 0);
}

//***************************************************************************
// add_loop
// Adds a loop point event to the stream.
//---------------------------------------------------------------------------
// param timestamp: event timestamp
//***************************************************************************

void add_loop(uint64_t timestamp)
{
   alloc_event(timestamp, 0, EV_LOOP, 0);
}

//***************************************************************************
// sort_stream
// Sorts all the events in the stream by timestamp (and other details).
//***************************************************************************

void sort_stream(void)
{
   qsort(events, num_events, sizeof(Event), sort_func);
}

static int sort_func(const void *ptr1, const void *ptr2) {
   // Cast pointers to the correct type
   const Event *ev1 = (const Event*) ptr1;
   const Event *ev2 = (const Event*) ptr2;

   // First of all: sort by timestamps!!
   if (ev1->timestamp < ev2->timestamp) return -1;
   if (ev1->timestamp > ev2->timestamp) return 1;

   // Sort by other smaller details
   if (ev1->channel < ev2->channel) return -1;
   if (ev1->channel > ev2->channel) return 1;
   if (ev1->type < ev2->type) return 1;
   if (ev1->type > ev2->type) return -1;

   // OK no idea what else to do... keep them as-is I guess?
   return -1;
}

//***************************************************************************
// get_num_events
// Retrieves how many events are in the stream.
//---------------------------------------------------------------------------
// return: number of events
//***************************************************************************

size_t get_num_events(void)
{
   return num_events;
}

//***************************************************************************
// get_event
// Retrieves the data for an event.
//---------------------------------------------------------------------------
// param id: event ID
// return: event pointer
//***************************************************************************

const Event *get_event(size_t id)
{
   return &events[id];
}
