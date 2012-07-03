//***************************************************************************
// "midi.c"
// MIDI-specific stuff, including parsing MIDI files
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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "main.h"
#include "event.h"
#include "midi.h"

// Possible chunk types
#define CHUNK_HEADER    0x4D546864     // "MThd" (header)
#define CHUNK_TRACK     0x4D54726B     // "MTrk" (track)

// Information in a chunk
typedef struct {
   uint32_t type;       // Type of chunk
   uint32_t size;       // Size in bytes
   uint8_t *data;       // Chunk data
} Chunk;

// Information for MIDI timing
// More complex than it could due to SMPTE...
typedef struct {
   unsigned smpte: 1;   // 0 = ticks per beat, 1 = SMPTE
   uint32_t ticks;      // Ticks per beat (normal) or per frame (SMPTE)
   uint32_t speed;      // Tempo (normal) or frames per 100 seconds (SMPTE)
   uint64_t last;       // Last timestamp (0x10000 = 1 frame)
} MidiTiming;

// Private function prototypes
static int read_chunk(FILE *, Chunk *);
static void free_chunk(Chunk *);
static int32_t read_varlen(uint8_t **, size_t *);
static void calculate_timestamp(int32_t, MidiTiming *);
static int calculate_volume(int, int);

// A list of MIDI-to-Echo channel mappings
static int channel_map[NUM_MIDICHAN];

// A list of MIDI-to-Echo instrument mappings
// There are different lists for each instrument
static struct {
   int instrument;      // Echo instrument (-1 = not mapped)
   int transpose;       // Transpose (in semitones)
   int volume;          // Volume scaling (percentage)
} instr_map[NUM_INSTRTYPES][NUM_MIDIINSTR];

// Status of each MIDI channel as we parse stuff
// Putting this here because otherwise it'll get lost in the middle of the
// MIDI code madness...
static struct {
   int instrument;      // Current instrument (-1 = none)
   int volume;          // Current channel volume
   int velocity;        // Current note volume
   int panning;         // Current panning
   int note;            // Last played note (-1 = none)
} status[NUM_MIDICHAN];

// Value used to convert the value of a pitch wheel event into semitones
static int pitch_factor;

//***************************************************************************
// read_midi
// Reads the data from a MIDI file and gathers the events from it.
//---------------------------------------------------------------------------
// param filename: input filename
// return: error code
//---------------------------------------------------------------------------
// Sorry for all the #ifdef DEBUG madness, but trying to spot out errors in a
// MIDI parser is really hard (it's very easy to miss something). If we find
// a MIDI that is considered corrupt by the tool but isn't corrupt, we can
// run it through a debug build to see what's wrong exactly.
//***************************************************************************

int read_midi(const char *filename) {
   // To store error codes
   int errcode;

   // Initialize event list
   reset_events();

   // Open input file
   FILE *file = fopen(filename, "rb");
   if (file == NULL)
      return ERR_OPENMIDI;

   // Read MIDI header
   Chunk chunk;
   errcode = read_chunk(file, &chunk);
   if (errcode) goto error;

   // Check that the header is valid
   if (chunk.type != CHUNK_HEADER) {
#ifdef DEBUG
      fprintf(stderr, "DEBUG: first chunk is type %04X\n", chunk.type);
#endif
      errcode = ERR_CORRUPT;
      goto error;
   }
   if (chunk.size != 6) {
#ifdef DEBUG
      fprintf(stderr, "DEBUG: chunk header size is %u\n", chunk.size);
#endif
      errcode = ERR_CORRUPT;
      goto error;
   }

   // Parse MIDI header
   uint16_t midi_type = chunk.data[0] << 8 | chunk.data[1];
   uint16_t num_tracks = chunk.data[2] << 8 | chunk.data[3];

   // Check that the MIDI type and such are correct
   if (midi_type > 2) {
#ifdef DEBUG
      fprintf(stderr, "DEBUG: MIDI type is %u\n", midi_type);
#endif
      errcode = ERR_CORRUPT;
      goto error;
   }
   if (midi_type == 0 && num_tracks != 1) {
#ifdef DEBUG
      fprintf(stderr, "DEBUG: MIDI type 0 with %u tracks\n", num_tracks);
#endif
      errcode = ERR_CORRUPT;
      goto error;
   }
   if (midi_type == 2) {
      errcode = ERR_MIDITYPE2;
      goto error;
   }

   // Parse timing information
   // I so hate this part...
   MidiTiming timing;
   if (chunk.data[4] & 0x80) {
      // SMPTE timing, use SMPTE timecode
      timing.smpte = 1;
      timing.ticks = chunk.data[5];

      // Determine framerate
      // Only a few framerates are allowed
      // Also WTF at 29 actually being 29.97
      switch (chunk.data[4] & 0x7F) {
         case 24: timing.speed = 2400; break;
         case 25: timing.speed = 2500; break;
         case 29: timing.speed = 2997; break;
         case 30: timing.speed = 3000; break;

         // Oops, invalid timing!
         default:
#ifdef DEBUG
            fprintf(stderr, "DEBUG: SMPTE framerate is %u\n",
            chunk.data[4] & 0x7F);
#endif
            errcode = ERR_CORRUPT;
            goto error;
      }
   } else {
      // Normal timing, specify in ticks per beat
      timing.smpte = 0;
      timing.ticks = chunk.data[4] << 8 | chunk.data[5];
      timing.speed = 120;
   }

   // Done with the header
   free_chunk(&chunk);

   // Start parsing the remaining chunks
   // We only care about track chunks, ignore the rest
   for (;;) {
      // Read next chunk
      errcode = read_chunk(file, &chunk);
      if (errcode) goto error;
      if (feof(file)) break;

      // Not a track?
      if (chunk.type != CHUNK_TRACK) {
         free_chunk(&chunk);
         continue;
      }

      // Reset timestamp
      // Each track starts at the first frame
      timing.last = 0;

      // Reset the status of all MIDI channels
      // This is just internal to send extra information for each event. That
      // said, this method can lead to issues with some unusual MIDIs (though
      // should work with most MIDIs out there). The issue is that the proper
      // way to handle MIDIs is to parse all tracks simultaneously...
      for (unsigned i = 0; i < NUM_MIDICHAN; i++) {
         status[i].instrument = -1;
         status[i].volume = 0x7F;
         status[i].velocity = 0x7F;
         status[i].panning = 0x40;
         status[i].note = -1;
      }

      // To keep track of running events
      // A value of 0 means no running events are valid
      uint8_t running_event = 0;

      // Parse MIDI arguments
      uint8_t *ptr = chunk.data;
      size_t size = chunk.size;
      while (size > 0) {
         // Read delta time for this event
         int32_t delta = read_varlen(&ptr, &size);
         if (delta == -1) {
#ifdef DEBUG
            fputs("DEBUG: invalid delta time\n", stderr);
#endif
            errcode = ERR_CORRUPT;
            goto error;
         }

         // Calculate timestamp for this event
         calculate_timestamp(delta, &timing);

         // Get next MIDI event
         uint8_t event = *ptr;
         if (event < 0x80) {
            event = running_event;
            if (!event) {
#ifdef DEBUG
               fprintf(stderr, "DEBUG: invalid MIDI event %02X\n", *ptr);
#endif
               errcode = ERR_CORRUPT;
               goto error;
            }
         } else {
            ptr++;
            size--;
         }

         // If this is a voice event, then keep it for running events. If
         // it isn't a voice event then disable running events.
         if (event >= 0xF0)
            running_event = 0;
         else
            running_event = event;

         // Note off?
         if (event >= 0x80 && event <= 0x8F) {
            // Make sure there are enough bytes left
            if (size < 2) {
#ifdef DEBUG
               fprintf(stderr, "DEBUG: ran out of bytes for event "
               "%02X\n", event);
#endif
               errcode = ERR_CORRUPT;
               goto error;
            }

            // Sanity check
            if (ptr[0] > 0x7F || ptr[1] > 0x7F) {
#ifdef DEBUG
               fprintf(stderr, "DEBUG: event %02X has invalid arguments "
               "(%02X %02X)\n", event, ptr[0], ptr[1]);
#endif
               errcode = ERR_CORRUPT;
               goto error;
            }

            // Issue a note off event for this channel
            Event *e = add_event(timing.last);
            if (e == NULL) {
               errcode = ERR_NOMEMORY;
               goto error;
            }
            e->type = EVENT_NOTEOFF;
            e->channel = channel_map[event & 0x0F];

            // Go for next event
            ptr += 2;
            size -= 2;
         }

         // Note on?
         else if (event >= 0x90 && event <= 0x9F) {
            // Make sure there are enough bytes left
            if (size < 2) {
#ifdef DEBUG
               fprintf(stderr, "DEBUG: ran out of bytes for event "
               "%02X\n", event);
#endif
               errcode = ERR_CORRUPT;
               goto error;
            }

            // Sanity check
            if (ptr[0] > 0x7F || ptr[1] > 0x7F) {
#ifdef DEBUG
               fprintf(stderr, "DEBUG: event %02X has invalid arguments "
               "(%02X %02X)\n", event, ptr[0], ptr[1]);
#endif
               errcode = ERR_CORRUPT;
               goto error;
            }

            // If velocity is 0, then this event behaves like note off
            // This was done since it's pretty common to have long runs of
            // note on and note off, and making it this way allows for
            // compressing all that into running events even though they're
            // different types of event.
            if (ptr[1] == 0x00) {
               // Issue a note off event for this channel
               Event *e = add_event(timing.last);
               if (e == NULL) {
                  errcode = ERR_NOMEMORY;
                  goto error;
               }
               e->type = EVENT_NOTEOFF;
               e->channel = channel_map[event & 0x0F];
            }

            // If velocity isn't 0 then it's a normal note on event, keep
            // parsing it as usual.
            else {
               // Get target channel
               int channel = channel_map[event & 0x0F];

               // Get note to play
               int note = ptr[0];

               // Calculate volume
               status[event & 0x0F].velocity = ptr[1];
               int volume = calculate_volume(event & 0x0F, channel);

               // Get instrument to use
               int instrument = -1;
               if (channel >= CHAN_FM1 && channel <= CHAN_FM6) {
                  instrument = instr_map[INSTR_FM]
                     [status[event & 0x0F].instrument].instrument;
                  note += instr_map[INSTR_FM]
                     [status[event & 0x0F].instrument].transpose;
               } else if (channel >= CHAN_PSG1 && channel <= CHAN_PSG4EX) {
                  instrument = instr_map[INSTR_PSG]
                     [status[event & 0x0F].instrument].instrument;
                  note += instr_map[INSTR_PSG]
                     [status[event & 0x0F].instrument].transpose;
               } else if (channel == CHAN_PCM)
                  instrument = instr_map[INSTR_PCM][note].instrument;

               // Issue a note on event for this channel
               Event *e = add_event(timing.last);
               if (e == NULL) {
                  errcode = ERR_NOMEMORY;
                  goto error;
               }
               e->type = EVENT_NOTEON;
               e->param = (channel == CHAN_PCM) ? instrument : note;
               e->channel = channel;
               e->instrument = instrument;
               e->volume = volume;
               e->panning = status[event & 0x0F].panning;

               // Keep track of which semitone is playing (needed for slides
               // to work properly)
               status[event & 0x0F].note = note << 4;
            }

            // Go for next event
            ptr += 2;
            size -= 2;
         }

         // Note aftertouch?
         // The name is misleading... aftertouch affects the "velocity" of
         // the note, which is actually the individual volume of said note!
         // Here we just assume it for the whole channel since polyphony is
         // undefined for midi2esf (as the hardware can't do that)
         else if (event >= 0xA0 && event <= 0xAF) {
            // Make sure there are enough bytes left
            if (size < 2) {
#ifdef DEBUG
               fprintf(stderr, "DEBUG: ran out of bytes for event "
               "%02X\n", event);
#endif
               errcode = ERR_CORRUPT;
               goto error;
            }

            // Sanity check
            if (ptr[0] > 0x7F || ptr[1] > 0x7F) {
#ifdef DEBUG
               fprintf(stderr, "DEBUG: event %02X has invalid arguments "
               "(%02X %02X)\n", event, ptr[0], ptr[1]);
#endif
               errcode = ERR_CORRUPT;
               goto error;
            }

            // Get target channel
            int channel = channel_map[event & 0x0F];

            // Calculate new volume
            status[event & 0x0F].velocity = ptr[1];
            int volume = calculate_volume(event & 0x0F, channel);

            // Issue a volume change event for this channel
            Event *e = add_event(timing.last);
            if (e == NULL) {
               errcode = ERR_NOMEMORY;
               goto error;
            }
            e->channel = channel;
            e->type = EVENT_VOLUME;
            e->param = volume;

            // Go for next event
            ptr += 2;
            size -= 2;
         }

         // Controller event?
         // This thing controls a lot of parameters. We only care about the
         // volume and panning parameters and ignore the rest.
         else if (event >= 0xB0 && event <= 0xBF) {
            // Make sure there are enough bytes left
            if (size < 2) {
#ifdef DEBUG
               fprintf(stderr, "DEBUG: ran out of bytes for event "
               "%02X\n", event);
#endif
               errcode = ERR_CORRUPT;
               goto error;
            }

            // Sanity check
            if (ptr[0] > 0x7F || ptr[1] > 0x7F) {
#ifdef DEBUG
               fprintf(stderr, "DEBUG: event %02X has invalid arguments "
               "(%02X %02X)\n", event, ptr[0], ptr[1]);
#endif
               errcode = ERR_CORRUPT;
               goto error;
            }

            // Check what kind of parameter to change
            // Ignore parameters we don't handle
            switch (ptr[0]) {
               // Channel volume change
               case 0x07: {
                  // Store new channel volume
                  status[event & 0x0F].volume = ptr[1];

                  // Get target channel
                  int channel = channel_map[event & 0x0F];

                  // Calculate new volume
                  status[event & 0x0F].velocity = ptr[1];
                  int volume = calculate_volume(event & 0x0F, channel);

                  // Issue a volume change event for this channel
                  Event *e = add_event(timing.last);
                  if (e == NULL) {
                     errcode = ERR_NOMEMORY;
                     goto error;
                  }
                  e->channel = channel;
                  e->type = EVENT_VOLUME;
                  e->param = volume;
               }; break;

               // Panning change
               case 0x10: {
                  // Store new panning position
                  status[event & 0x0F].panning = ptr[1];

                  // Get target channel
                  int channel = channel_map[event & 0x0F];

                  // Issue a panning change event for this channel
                  Event *e = add_event(timing.last);
                  if (e == NULL) {
                     errcode = ERR_NOMEMORY;
                     goto error;
                  }
                  e->channel = channel;
                  e->type = EVENT_PAN;
                  e->param = ptr[1];
               }; break;

               // Not handled, ignore...
               default:
                  break;
            }

            // Go for next event
            ptr += 2;
            size -= 2;
         }

         // Program change? (instrument change)
         else if (event >= 0xC0 && event <= 0xCF) {
            // Make sure there are enough bytes left
            if (size < 1) {
#ifdef DEBUG
               fprintf(stderr, "DEBUG: ran out of bytes for event "
               "%02X\n", event);
#endif
               errcode = ERR_CORRUPT;
               goto error;
            }

            // Sanity check
            if (ptr[0] > 0x7F) {
#ifdef DEBUG
               fprintf(stderr, "DEBUG: event %02X has invalid arguments "
               "(%02X)\n", event, ptr[0]);
#endif
               errcode = ERR_CORRUPT;
               goto error;
            }

            // Store new instrument
            status[event & 0x0F].instrument = ptr[0];

            // Go for next event
            ptr++;
            size--;
         }

         // Channel aftertouch?
         // Similar deal as with note aftertouch...
         else if (event >= 0xD0 && event <= 0xDF) {
            // Make sure there are enough bytes left
            if (size < 1) {
#ifdef DEBUG
               fprintf(stderr, "DEBUG: ran out of bytes for event "
               "%02X\n", event);
#endif
               errcode = ERR_CORRUPT;
               goto error;
            }

            // Sanity check
            if (ptr[0] > 0x7F) {
#ifdef DEBUG
               fprintf(stderr, "DEBUG: event %02X has invalid arguments "
               "(%02X)\n", event, ptr[0]);
#endif
               errcode = ERR_CORRUPT;
               goto error;
            }

            // Get target channel
            int channel = channel_map[event & 0x0F];

            // Calculate new volume
            status[event & 0x0F].velocity = ptr[0];
            int volume = calculate_volume(event & 0x0F, channel);

            // Issue a volume change event for this channel
            Event *e = add_event(timing.last);
            if (e == NULL) {
               errcode = ERR_NOMEMORY;
               goto error;
            }
            e->channel = channel;
            e->type = EVENT_VOLUME;
            e->param = volume;

            // Go for next event
            ptr++;
            size--;
         }

         // Pitch wheel? (used for note slides)
         else if (event >= 0xE0 && event <= 0xEF) {
            // Make sure there are enough bytes left
            if (size < 2) {
#ifdef DEBUG
               fprintf(stderr, "DEBUG: ran out of bytes for event "
               "%02X\n", event);
#endif
               errcode = ERR_CORRUPT;
               goto error;
            }

            // Sanity check
            if (ptr[0] > 0x7F || ptr[1] > 0x7F) {
#ifdef DEBUG
               fprintf(stderr, "DEBUG: event %02X has invalid arguments "
               "(%02X %02X)\n", event, ptr[0], ptr[1]);
#endif
               errcode = ERR_CORRUPT;
               goto error;
            }

            // Get target channel
            int channel = channel_map[event & 0x0F];

            // Calculate note to play (in 1/16ths of a semitone)
            int wheel = ptr[1] << 7 | ptr[0];
            /*int note = status[event & 0x0F].note +
                       (wheel - 0x2000) / 0x100;*/
            int note = status[event & 0x0F].note +
                       (wheel - 0x2000) / pitch_factor;

            // Issue a pitch change event for this channel
            Event *e = add_event(timing.last);
            if (e == NULL) {
               errcode = ERR_NOMEMORY;
               goto error;
            }
            e->channel = channel;
            e->type = EVENT_SLIDE;
            e->param = note;

            // Go for next event
            ptr += 2;
            size -= 2;
         }

         // Ignore SysEx events
         else if (event == 0xF0 || event == 0xF7) {
            // Get length of event
            int32_t len = read_varlen(&ptr, &size);
            if (len == -1) {
#ifdef DEBUG
               fputs("DEBUG: invalid length for SysEx event\n", stderr);
#endif
               errcode = ERR_CORRUPT;
               goto error;
            }

            // Skip bytes as needed
            if ((uint32_t)(len) > size) {
#ifdef DEBUG
               fputs("DEBUG: ran out of bytes for SysEx event\n", stderr);
#endif
               errcode = ERR_CORRUPT;
               goto error;
            }
            ptr += len;
            size -= len;
         }

         // Meta event?
         else if (event == 0xFF) {
            // Get meta event type
            if (size < 1) {
#ifdef DEBUG
               fputs("DEBUG: ran out of bytes for meta event "
                     "(missing type)\n", stderr);
#endif
               errcode = ERR_CORRUPT;
               goto error;
            }
            uint8_t type = *ptr++;
            size--;

            // Get length of meta event in bytes
            int32_t len = read_varlen(&ptr, &size);
            if (len == -1) {
#ifdef DEBUG
               fputs("Invalid length for meta event\n", stderr);
#endif
               errcode = ERR_CORRUPT;
               break;
            }

            // Make sure we have enough bytes left for the meta event
            if (size < (size_t) len) {
#ifdef DEBUG
               fprintf(stderr, "DEBUG: ran out of bytes for meta event "
                  "%02X\n", type);
#endif
               errcode = ERR_CORRUPT;
               break;
            }

            // Determine what to do with this meta event
            // If we don't know or don't care just ignore it
            switch (type) {
               // Cue point?
               // (unhandled for now, could be used later to specify the loop
               // point for looping BGM streams)
               case 0x07:
                  break;

               // Change tempo?
               case 0x51:
                  // ...
                  // To-do: handle meta event
                  // ...
                  break;

               // SMPTE offset?
               case 0x54:
                  // Should be at the beginning only...
                  if (timing.last != 0)
                     break;

                  // ...
                  // To-do: handle meta event
                  // ...
                  break;

               // Ignore meta event...
               default:
                  break;
            }

            // Go for next event
            ptr += len;
            size -= len;
         }

         // Unhandled event?
         else {
#ifdef DEBUG
            fprintf(stderr, "DEBUG: unknown event %02X\n", event);
#endif
            errcode = ERR_CORRUPT;
            goto error;
         }
      }

      // Done with chunk
      free_chunk(&chunk);
   }

   // Success!
   free_chunk(&chunk);
   fclose(file);
   return ERR_NONE;

   // Failure...
error:
   free_chunk(&chunk);
   fclose(file);
   return errcode;
}

//***************************************************************************
// read_chunk [internal]
// Reads a chunk from a MIDI file into memory.
//---------------------------------------------------------------------------
// param file: input file
// param chunk: where chunk data will be stored
// return: error code
//***************************************************************************

static int read_chunk(FILE *file, Chunk *chunk) {
   // Reset chunk information
   chunk->type = 0;
   chunk->size = 0;
   chunk->data = NULL;

   // Read the chunk header
   uint8_t buffer[8];
   int size_read = fread(&buffer, 1, 8, file);
   if (size_read == 0 && feof(file))
      return ERR_NONE;
   else if (size_read < 8)
      return feof(file) ? ERR_CORRUPT : ERR_READMIDI;

   // Parse the chunk header
   chunk->type = buffer[0] << 24 |
                 buffer[1] << 16 |
                 buffer[2] << 8 |
                 buffer[3];

   chunk->size = buffer[4] << 24 |
                 buffer[5] << 16 |
                 buffer[6] << 8 |
                 buffer[7];

   // Empty chunk? (if so, done here)
   if (chunk->size == 0)
      return ERR_NONE;

   // Allocate memory for the chunk data
   chunk->data = (uint8_t *) malloc(chunk->size);
   if (chunk->data == NULL)
      return ERR_NOMEMORY;

   // Read chunk data from the file
   if (fread(chunk->data, 1, chunk->size, file) < chunk->size) {
      free(chunk->data);
      chunk->data = NULL;
      return feof(file) ? ERR_CORRUPT : ERR_READMIDI;
   }

   // Success!
   return ERR_NONE;
}

//***************************************************************************
// free_chunk [internal]
// Deallocates the memory used by a chunk, if needed.
//---------------------------------------------------------------------------
// param chunk: where chunk data is stored
//***************************************************************************

static void free_chunk(Chunk *chunk) {
   // Deallocate data if needed
   if (chunk->data != NULL)
      free(chunk->data);

   // Reset chunk data
   chunk->type = 0;
   chunk->size = 0;
   chunk->data = NULL;
}

//***************************************************************************
// read_varlen [internal]
// Reads a variable length number from a chunk.
//---------------------------------------------------------------------------
// param data: pointer to variable with pointer to data being read
// param size: pointer to variable with amount of remaining bytes
// return: value being read, or -1 on error
//         updates value of *data and *size on success
//***************************************************************************

static int32_t read_varlen(uint8_t **data, size_t *size) {
   // For the sake of making our lives easier
   // ptr == pointer to data being read
   uint8_t *ptr = *data;

   // Single byte value?
   if (*size >= 1 && ptr[0] < 0x80) {
      *data += 1;
      *size -= 1;
      return *ptr;
   }

   // Two bytes value?
   else if (*size >= 2 && ptr[1] < 0x80) {
      *data += 2;
      *size -= 2;
      return (ptr[0] & 0x7F) << 7 | ptr[1];
   }

   // Three bytes value?
   else if (*size >= 3 && ptr[2] < 0x80) {
      *data += 3;
      *size -= 3;
      return (ptr[0] & 0x7F) << 14 |
             (ptr[1] & 0x7F) << 7 |
              ptr[2];
   }

   // Four bytes value?
   else if (*size >= 4 && ptr[3] < 0x80) {
      *data += 4;
      *size -= 4;
      return (ptr[0] & 0x7F) << 21 |
             (ptr[1] & 0x7F) << 14 |
             (ptr[2] & 0x7F) << 7 |
              ptr[3];
   }

   // Invalid sequence?
   else
      return -1;
}

//***************************************************************************
// map_channel
// Maps a MIDI channel to an Echo channel.
//---------------------------------------------------------------------------
// param midichan: MIDI channel (1 to 128)
// param echochan: Echo channel (see CHAN_*)
//***************************************************************************

void map_channel(int midichan, int echochan) {
   channel_map[midichan - 1] = echochan;
}

//***************************************************************************
// map_instrument
// Maps a MIDI instrument to an Echo instrument.
//---------------------------------------------------------------------------
// param type: instrument type (see INSTR_*)
// param midiinstr: MIDI instrument (0 to 127)
// param echoinstr: Echo instrument (0 to 255)
// param transpose: transpose (in semitones)
// param volume: volume scale (percentage)
//***************************************************************************

void map_instrument(int type, int midiinstr, int echoinstr, int transpose,
int volume) {
   instr_map[type][midiinstr].instrument = echoinstr;
   instr_map[type][midiinstr].transpose = transpose;
   instr_map[type][midiinstr].volume = volume;
}

//***************************************************************************
// set_pitch_range
// Sets the range for pitch wheel. The range is given in the amount of
// semitones it can go up/down (default: 2).
//---------------------------------------------------------------------------
// param range: amount of semitones
//***************************************************************************

void set_pitch_range(int range) {
   pitch_factor = 0x200 / range;
}

//***************************************************************************
// calculate_timestamp [internal]
// Calculates the timestamp of an event (in Echo ticks!)
//---------------------------------------------------------------------------
// param delta: delta time for this event
// param timing: structure with timing information
// return: timestamp for this event (in Echo ticks, 48.16 fixed comma)
//***************************************************************************

// These constants are pretty much well-defined (and not by our program), but
// I'm naming them just to make it clearer what's the logic behind the timing
// calculations.
#define ECHO_TICKRATE 60      // Echo's tickrate (60Hz)
#define SECS_PER_MIN 60       // Seconds in a minute (60, d'oh!)

static void calculate_timestamp(int32_t delta, MidiTiming *timing) {
   // Huh, no timestamp change?
   // (checking this explicitly since it's very common)
   if (delta == 0)
      return;

   // To store how much of a difference is there from the last timestamp
   uint64_t value;

   // SMPTE timing
   // Separating this in individual lines because it's a mess... The compiler
   // will eventually notice the potential optimizations anyways, it isn't
   // that stupid as far as I know.
   if (timing->smpte) {
      value = delta;
      value *= ECHO_TICKRATE * 100 << 16;
      value /= timing->speed;
      value /= timing->ticks;
   }

   // Normal timing
   // Again, split into multiple lines to make it clearer.
   else {
      value = delta;
      value *= ECHO_TICKRATE * SECS_PER_MIN << 16;
      value /= timing->speed;
      value /= timing->ticks;
   }

   // Update timestamp
   timing->last += value;
}

//***************************************************************************
// calculate_volume [internal]
// Returns the output volume (to pass to the ESF parser) of the specified
// channel.
//---------------------------------------------------------------------------
// param midichan: MIDI channel
// param echochan: Echo channel
// return: output volume (0..127)
//***************************************************************************

static int calculate_volume(int midichan, int echochan) {
   // Calculate the actual volume based on all the parameters (thanks MIDI!)
   int output = status[midichan].volume *
      status[midichan].velocity / 0x7F;

   // Apply instrument's volume scaling
   if (echochan >= CHAN_FM1 && echochan <= CHAN_FM6) {
      output = output * instr_map[INSTR_FM]
      [status[midichan].instrument].volume / 100;
   } else if (echochan >= CHAN_PSG1 && echochan <= CHAN_PSG4EX) {
      output = output * instr_map[INSTR_PSG]
      [status[midichan].instrument].volume / 100;
   } else
      output = 0x7F;

   // Cap the volume to ensure it doesn't go past the limit
   if (output > 0x7F)
      output = 0x7F;
   if (output < 0x00)
      output = 0x00;

   // Return final volume
   return output;
}
