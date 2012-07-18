//***************************************************************************
// "echo.c"
// Echo-specific stuff, including conversion to ESF
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
#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "echo.h"
#include "event.h"

// Private function prototypes
static int write_noteon(FILE *, int, int);
static int write_noteoff(FILE *, int);
static int write_slide(FILE *, int, int);
static int write_instrument(FILE *, int, int);
static int write_volume(FILE *, int, int);
static int write_panning(FILE *, int, int);
static int write_delay(FILE *, uint64_t);
static int write_loopstart(FILE *file);
static int write_loopend(FILE *file);
static int write_end(FILE *);

// Table to convert MIDI volume into YM2612 volume
// (MIDI is linear, YM2612 is logaritmic)
static const uint8_t volume_fm[] = {
   0x7F, 0x7B, 0x78, 0x74, 0x71, 0x6E, 0x6B, 0x69,
   0x66, 0x64, 0x61, 0x5F, 0x5D, 0x5B, 0x59, 0x57,
   0x55, 0x53, 0x52, 0x50, 0x4E, 0x4D, 0x4B, 0x4A,
   0x48, 0x47, 0x45, 0x44, 0x43, 0x41, 0x40, 0x3F,
   0x3E, 0x3D, 0x3B, 0x3A, 0x39, 0x38, 0x37, 0x36,
   0x35, 0x34, 0x33, 0x32, 0x31, 0x30, 0x2F, 0x2E,
   0x2D, 0x2C, 0x2C, 0x2B, 0x2A, 0x29, 0x28, 0x27,
   0x27, 0x26, 0x25, 0x24, 0x24, 0x23, 0x22, 0x21,
   0x21, 0x20, 0x1F, 0x1F, 0x1E, 0x1D, 0x1D, 0x1C,
   0x1B, 0x1B, 0x1A, 0x19, 0x19, 0x18, 0x18, 0x17,
   0x16, 0x16, 0x15, 0x15, 0x14, 0x13, 0x13, 0x12,
   0x12, 0x11, 0x11, 0x10, 0x10, 0x0F, 0x0F, 0x0E,
   0x0E, 0x0D, 0x0D, 0x0C, 0x0C, 0x0B, 0x0B, 0x0A,
   0x0A, 0x09, 0x09, 0x08, 0x08, 0x08, 0x07, 0x07,
   0x06, 0x06, 0x05, 0x05, 0x04, 0x04, 0x04, 0x03,
   0x03, 0x02, 0x02, 0x02, 0x01, 0x01, 0x00, 0x00
};

// Table to convert MIDI volume into PSG volume
// (MIDI is linear, PSG is logaritmic)
static const uint8_t volume_psg[] = {
   0x0F, 0x0F, 0x0E, 0x0E, 0x0D, 0x0D, 0x0D, 0x0C,
   0x0C, 0x0C, 0x0C, 0x0B, 0x0B, 0x0B, 0x0B, 0x0A,
   0x0A, 0x0A, 0x0A, 0x09, 0x09, 0x09, 0x09, 0x09,
   0x09, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x07,
   0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x06, 0x06,
   0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x05,
   0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
   0x05, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
   0x04, 0x04, 0x04, 0x04, 0x04, 0x03, 0x03, 0x03,
   0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
   0x03, 0x03, 0x03, 0x02, 0x02, 0x02, 0x02, 0x02,
   0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
   0x02, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
   0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
   0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// FM frequencies for notes C to B
// This goes in steps of 1/16th of a semitone (for pitch slides)
// To determine the octave set the relevant bits
static const uint16_t fm_freq[] = {
    644,  646,  648,  651,  653,  655,  658,  660,    // C
    662,  665,  667,  670,  672,  674,  677,  679,
    682,  684,  687,  689,  692,  694,  697,  699,    // C#
    702,  704,  707,  709,  712,  715,  717,  720,
    722,  725,  728,  730,  733,  736,  738,  741,    // D
    744,  746,  749,  752,  754,  757,  760,  763,
    765,  768,  771,  774,  776,  779,  782,  785,    // D#
    788,  791,  794,  796,  799,  802,  805,  808,
    811,  814,  817,  820,  823,  826,  829,  832,    // E
    835,  838,  841,  844,  847,  850,  853,  856,
    859,  862,  865,  868,  872,  875,  878,  881,    // F
    884,  888,  891,  894,  897,  900,  904,  907,
    910,  914,  917,  920,  924,  927,  930,  934,    // F#
    937,  940,  944,  947,  951,  954,  957,  961,
    964,  968,  971,  975,  978,  982,  986,  989,    // G
    993,  996, 1000, 1003, 1007, 1011, 1014, 1018,
   1022, 1025, 1029, 1033, 1037, 1040, 1044, 1048,    // G#
   1052, 1056, 1059, 1063, 1067, 1071, 1075, 1079,
   1083, 1086, 1090, 1094, 1098, 1102, 1106, 1110,    // A
   1114, 1118, 1122, 1126, 1131, 1135, 1139, 1143,
   1147, 1151, 1155, 1159, 1164, 1168, 1172, 1176,    // A#
   1181, 1185, 1189, 1193, 1198, 1202, 1206, 1211,
   1215, 1220, 1224, 1228, 1233, 1237, 1242, 1246,    // B
   1251, 1255, 1260, 1264, 1269, 1274, 1278, 1283
};

// PSG frequencies for notes C-4 to B-4
// This goes in steps of 1/16th of a semitone (for pitch slides)
// For other octaves just divide as needed...
static const uint16_t psg_freq[] = {
    851,  847,  844,  841,  838,  835,  832,  829,    // C-4
    826,  823,  820,  817,  814,  811,  809,  806,
    803,  800,  797,  794,  791,  788,  786,  783,    // C#4
    780,  777,  774,  771,  769,  766,  763,  760,
    758,  755,  752,  749,  747,  744,  741,  739,    // D-4
    736,  733,  731,  728,  726,  723,  720,  718,
    715,  713,  710,  707,  705,  702,  700,  697,    // D#4
    695,  692,  690,  687,  685,  682,  680,  677,
    675,  673,  670,  668,  665,  663,  660,  658,    // E-4
    656,  653,  651,  649,  646,  644,  642,  639,
    637,  635,  632,  630,  628,  626,  623,  621,    // F-4
    619,  617,  614,  612,  610,  608,  606,  603,
    601,  599,  597,  595,  593,  590,  588,  586,    // F#4
    584,  582,  580,  578,  576,  574,  572,  570,
    567,  565,  563,  561,  559,  557,  555,  553,    // G-4
    551,  549,  547,  545,  543,  541,  539,  538,
    536,  534,  532,  530,  528,  526,  524,  522,    // G#4
    520,  518,  517,  515,  513,  511,  509,  507,
    506,  504,  502,  500,  498,  496,  495,  493,    // A-4
    491,  489,  488,  486,  484,  482,  481,  479,
    477,  475,  474,  472,  470,  469,  467,  465,    // A#4
    464,  462,  460,  459,  457,  455,  454,  452,
    450,  449,  447,  445,  444,  442,  441,  439,    // B-4
    437,  436,  434,  433,  431,  430,  428,  427
};

//***************************************************************************
// write_esf
// Parses the events and generates an ESF file.
//---------------------------------------------------------------------------
// param filename: output filename
// param loop: zero to play once, non-zero to loop forever
// return: error code
//***************************************************************************

int write_esf(const char *filename, int loop) {
   // To store error codes
   int errcode = ERR_NONE;

   // To keep track of the status of each Echo channel
   // Used for stream optimization purposes, etc.
   struct {
      int instrument;         // Last instrument used (-1 if none)
      int volume;             // Last volume used (-1 if none)
      int panning;            // Last panning used (-1 if none)
      int note;               // Last note used (in 1/16ths, -1 if none)
   } status[NUM_ECHOCHAN];

   // Initialize channel status to undefined
   for (unsigned i = 0; i < NUM_ECHOCHAN; i++) {
      status[i].instrument = -1;
      status[i].volume = -1;
      status[i].panning = -1;
      status[i].note = -1;
   }

   // Open output file
   FILE *file = fopen(filename, "wb");
   if (file == NULL)
      return ERR_OPENESF;

   // Looping stream?
   if (loop) {
      errcode = write_loopstart(file);
      if (errcode) goto error;
   }

   // Frame when the last event happened
   uint64_t last_time = 0;

   // Parse all events
   for (const Event *event = get_events(); event != NULL;
   event = event->next) {
      // Skip void events...
      if (event->channel == CHAN_NONE)
         continue;

      // Introduce delay?
      uint64_t curr_time = event->timestamp >> 16;
      if (curr_time > last_time) {
         // Generate delay events
         errcode = write_delay(file, curr_time - last_time);
         if (errcode) {
            fclose(file);
            return errcode;
         }

         // Update last timestamp
         last_time = curr_time;
      }

      // Determine channel panning (FM only)
      int panning;
      if (event->panning < 0x20)
         panning = 0x80;
      else if (event->panning >= 0x60)
         panning = 0x40;
      else
         panning = 0xC0;

      // Determine what action to take based on the event
      switch (event->type) {
         // Note on?
         case EVENT_NOTEON:
            // Ignore void events...
            if (event->instrument == -1)
               break;

#ifdef DEBUG
            printf("[%s] %lu on %d note %d ins %d\n",
               filename, event->timestamp >> 16,
               event->channel, event->param, event->instrument);
#endif

            // PSG3+PSG4 is a wacky combination that needs some extra
            // processing to ensure it's working properly... (more
            // specifically: PSG3 is mute and sets the note pitch, while PSG4
            // plays the notes)
            if (event->channel == CHAN_PSG4EX) {
               // Force PSG3 to be muted
               if (status[CHAN_PSG3].volume != 0x00) {
                  status[CHAN_PSG3].volume = 0x00;
                  errcode = write_volume(file, CHAN_PSG3, 0x00);
                  if (errcode) goto error;
               }

               // PSG3 will share the same instrument as PSG4 (so we can use
               // it with a valid instrument and let note on events work
               // properly - otherwise we have to use pitch slides which take
               // up more space)
               status[CHAN_PSG3].instrument = event->instrument;
            }

            // Instrument change?
            if (event->instrument != status[event->channel].instrument) {
               status[event->channel].instrument = event->instrument;
               status[event->channel].volume = -1;
               errcode = write_instrument(file, event->channel,
                  event->instrument);
               if (errcode) goto error;
            }

            // Volume change?
            if (event->volume != status[event->channel].volume) {
               status[event->channel].volume = event->volume;
               errcode = write_volume(file, event->channel, event->volume);
               if (errcode) goto error;
            }

            // Panning change?
            if (panning != status[event->channel].panning) {
               status[event->channel].panning = panning;
               errcode = write_panning(file, event->channel, panning);
               if (errcode) goto error;
            }

            // Store current note
            status[event->channel].note = event->param << 4;

            // Issue Echo note on event
            errcode = write_noteon(file, event->channel, event->param);
            if (errcode) goto error;
            break;

         // Note off?
         case EVENT_NOTEOFF:
            // Quick optimization: if the next event is a note on for this
            // same channel, throw away the note off (since Echo will handle
            // this on its own). It won't catch all the relevant cases, but
            // it should work with most MIDIs and will reduce the filesize
            // most of the time.
            if (event->next != NULL && event->next->type == EVENT_NOTEON &&
            event->next->channel == event->channel)
               break;

            // Mark note as not playing
            status[event->channel].note = -1;

            // Issue Echo note off event
            errcode = write_noteoff(file, event->channel);
            if (errcode) goto error;
            break;

         // Change pitch?
         case EVENT_SLIDE:
            // Don't do anything if the note isn't playing!
            if (status[event->channel].note == -1)
               break;

            // Check if a change in pitch is needed for starters
            // (do nothing if it wouldn't change)
            if (status[event->channel].note == event->param)
               break;

            // Store current note
            status[event->channel].note = event->param;

            // Issue Echo note slide event
            errcode = write_slide(file, event->channel, event->param);
            if (errcode) goto error;

            break;

         // Change volume?
         case EVENT_VOLUME:
            if (event->volume != status[event->channel].volume) {
               status[event->channel].volume = event->volume;
               errcode = write_volume(file, event->channel, event->volume);
               if (errcode) goto error;
            }
            break;

         // Change panning?
         case EVENT_PAN:
            if (event->panning != panning) {
               status[event->channel].panning = panning;
               errcode = write_panning(file, event->channel, panning);
               if (errcode) goto error;
            }
            break;

         // Shouldn't happen...
         default:
            errcode = ERR_UNKNOWN;
            break;
      }
   }

   // End the stream here
   if (loop)
      errcode = write_loopend(file);
   else
      errcode = write_end(file);
   if (errcode)
      goto error;

   // Success!
   fclose(file);
   return ERR_NONE;

   // Failure?
error:
   fclose(file);
   return errcode;
}

//***************************************************************************
// write_noteon [internal]
// Writes an event to note on a channel.
//---------------------------------------------------------------------------
// param file: output file
// param channel: affected channel
// param note: note to play
// return: error code
//***************************************************************************

static int write_noteon(FILE *file, int channel, int note) {
   // FM channel?
   if (channel >= CHAN_FM1 && channel <= CHAN_FM6) {
      // Clamp note to valid values
      note -= 12;
      if (note < 0) note = 0;
      if (note > 95) note = 95;

      // Determine output Echo channel
      uint8_t out_chan;
      switch (channel) {
         case CHAN_FM1: out_chan = ECHO_FM1; break;
         case CHAN_FM2: out_chan = ECHO_FM2; break;
         case CHAN_FM3: out_chan = ECHO_FM3; break;
         case CHAN_FM4: out_chan = ECHO_FM4; break;
         case CHAN_FM5: out_chan = ECHO_FM5; break;
         case CHAN_FM6: out_chan = ECHO_FM6; break;
         default: return ERR_UNKNOWN;
      }

      // Determine value for the Echo event parameter
      int octave = note / 12;
      int semitone = note % 12;
      uint8_t out_param = octave * 0x20 + semitone * 2 + 1;

      // Output event values
      if (fputc(ECHO_NOTEON | out_chan, file) == EOF)
         return ERR_WRITEESF;
      if (fputc(out_param, file) == EOF)
         return ERR_WRITEESF;
   }

   // Square wave PSG channel?
   else if (channel >= CHAN_PSG1 && channel <= CHAN_PSG3) {
      // Clamp note to valid values
      note -= 48;
      if (note < 0) note = 0;
      if (note > 59) note = 59;

      // Determine output Echo channel
      uint8_t out_chan;
      switch (channel) {
         case CHAN_PSG1: out_chan = ECHO_PSG1; break;
         case CHAN_PSG2: out_chan = ECHO_PSG2; break;
         case CHAN_PSG3: out_chan = ECHO_PSG3; break;
         default: return ERR_UNKNOWN;
      }

      // Determine value for the Echo event parameter
      int octave = note / 12;
      int semitone = note % 12;
      uint8_t out_param = octave * 24 + semitone * 2;

      // Output event values
      if (fputc(ECHO_NOTEON | out_chan, file) == EOF)
         return ERR_WRITEESF;
      if (fputc(out_param, file) == EOF)
         return ERR_WRITEESF;
   }

   // Noise PSG channel? (standard mode)
   else if (channel == CHAN_PSG4) {
      // Clamp note to valid values
      note -= 48;
      note /= 12;
      if (note < 0) note = 0;
      if (note > 2) note = 2;

      // The lower the value, the higher the pitch
      // So, get the correct value for this octave
      note = 2 - note;

      // Output event values
      if (fputc(ECHO_NOTEON | ECHO_PSG4, file) == EOF)
         return ERR_WRITEESF;
      if (fputc(note + 4, file) == EOF)
         return ERR_WRITEESF;
   }

   // Noise PSG channel? (PSG3 mode)
   else if (channel == CHAN_PSG4EX) {
      // Issue note on events
      int errcode = write_noteon(file, CHAN_PSG3, note);
      if (errcode) return errcode;

      if (fputc(ECHO_NOTEON | ECHO_PSG4, file) == EOF)
         return ERR_WRITEESF;
      if (fputc(0x07, file) == EOF)
         return ERR_WRITEESF;
   }

   // PCM channel?
   else if (channel == CHAN_PCM) {
      // Output event values as-is, since they were already pre-processed
      if (fputc(ECHO_NOTEON | ECHO_PCM, file) == EOF)
         return ERR_WRITEESF;
      if (fputc(note, file) == EOF)
         return ERR_WRITEESF;
   }

   // Success!
   return ERR_NONE;
}

//***************************************************************************
// write_noteoff [internal]
// Writes an event to note off a channel.
//---------------------------------------------------------------------------
// param file: output file
// param channel: affected channel
// return: error code
//***************************************************************************

static int write_noteoff(FILE *file, int channel) {
   // Determine which Echo channel gets the note off
   switch (channel) {
      case CHAN_FM1: channel = ECHO_FM1; break;
      case CHAN_FM2: channel = ECHO_FM2; break;
      case CHAN_FM3: channel = ECHO_FM3; break;
      case CHAN_FM4: channel = ECHO_FM4; break;
      case CHAN_FM5: channel = ECHO_FM5; break;
      case CHAN_FM6: channel = ECHO_FM6; break;
      case CHAN_PSG1: channel = ECHO_PSG1; break;
      case CHAN_PSG2: channel = ECHO_PSG2; break;
      case CHAN_PSG3: channel = ECHO_PSG3; break;
      case CHAN_PSG4: channel = ECHO_PSG4; break;
      case CHAN_PSG4EX: channel = ECHO_PSG4; break;
      case CHAN_PCM: channel = ECHO_PCM; break;

      case CHAN_NONE: return ERR_NONE;
      default: return ERR_UNKNOWN; break;
   }

   // Write note off event
   if (fputc(ECHO_NOTEOFF | channel, file) == EOF)
      return ERR_WRITEESF;

   // Success!
   return ERR_NONE;
}

//***************************************************************************
// write_slide [internal]
// Writes an event to do a note slide on a channel.
//---------------------------------------------------------------------------
// param file: output file
// param channel: affected channel
// param note: note to play (in 1/16ths semitones)
// return: error code
//***************************************************************************

static int write_slide(FILE *file, int channel, int note) {
   // FM channel?
   if (channel >= CHAN_FM1 && channel <= CHAN_FM6) {
      // Clamp note to valid values
      note -= 12 << 4;
      if (note < 0) note = 0;
      if (note > (95 << 4)) note = (95 << 4);

      // Determine output Echo channel
      uint8_t out_chan;
      switch (channel) {
         case CHAN_FM1: out_chan = ECHO_FM1; break;
         case CHAN_FM2: out_chan = ECHO_FM2; break;
         case CHAN_FM3: out_chan = ECHO_FM3; break;
         case CHAN_FM4: out_chan = ECHO_FM4; break;
         case CHAN_FM5: out_chan = ECHO_FM5; break;
         case CHAN_FM6: out_chan = ECHO_FM6; break;
         default: return ERR_UNKNOWN;
      }

      // Determine event parameter
      unsigned octave = (note >> 4) / 12;
      unsigned index = note % (12 << 4);
      uint16_t freq = fm_freq[index] | (octave << 11);

      // Output event values
      if (fputc(ECHO_FREQ | out_chan, file) == EOF)
         return ERR_WRITEESF;
      if (fputc(freq >> 8, file) == EOF)
         return ERR_WRITEESF;
      if (fputc(freq & 0xFF, file) == EOF)
         return ERR_WRITEESF;
   }

   // Square wave PSG channel?
   else if ((channel >= CHAN_PSG1 && channel <= CHAN_PSG3) ||
   channel == CHAN_PSG4EX) {
      // Clamp note to valid values
      note -= 48 << 4;
      if (note < 0) note = 0;
      if (note > (59 << 4)) note = (59 << 4);

      // Determine output Echo channel
      uint8_t out_chan;
      switch (channel) {
         case CHAN_PSG1: out_chan = ECHO_PSG1; break;
         case CHAN_PSG2: out_chan = ECHO_PSG2; break;
         case CHAN_PSG3: out_chan = ECHO_PSG3; break;
         case CHAN_PSG4EX: out_chan = ECHO_PSG3; break;
         default: return ERR_UNKNOWN;
      }

      // Determine event parameter
      unsigned octave = (note >> 4) / 12;
      uint16_t freq = psg_freq[note % (12 << 4)];
      freq >>= octave;

      // Output event values
      if (fputc(ECHO_FREQ | out_chan, file) == EOF)
         return ERR_WRITEESF;
      if (fputc(freq & 0x0F, file) == EOF)
         return ERR_WRITEESF;
      if (fputc(freq >> 4, file) == EOF)
         return ERR_WRITEESF;
   }

   // Noise PSG channel? (standard mode)
   else if (channel == CHAN_PSG4) {
      // Clamp note to valid values
      note -= 48 << 4;
      note /= 12 << 4;
      if (note < 0) note = 0;
      if (note > 2) note = 2;

      // The lower the value, the higher the pitch
      // So, get the correct value for this octave
      note = 2 - note;

      // Output event values
      if (fputc(ECHO_FREQ | ECHO_PSG4, file) == EOF)
         return ERR_WRITEESF;
      if (fputc(note, file) == EOF)
         return ERR_WRITEESF;
   }

   // Success!
   return ERR_NONE;
}

//***************************************************************************
// write_instrument [internal]
// Writes the event to change a channel instrument.
//---------------------------------------------------------------------------
// param file: output file
// param channel: affected channel
// param instrument: new instrument
// return: error code
//***************************************************************************

static int write_instrument(FILE *file, int channel, int instrument) {
   // Make PSG3 share the same instrument as PSG4 in the PSG3 noise mode.
   // This is so we can safely use note on with it and save space in the
   // stream instead of having to use slide all the time.
   if (channel == CHAN_PSG4EX) {
      int errcode = write_instrument(file, CHAN_PSG3, instrument);
      if (errcode) return errcode;
   }

   // Determine which Echo channel gets the instrument change
   switch (channel) {
      case CHAN_FM1: channel = ECHO_FM1; break;
      case CHAN_FM2: channel = ECHO_FM2; break;
      case CHAN_FM3: channel = ECHO_FM3; break;
      case CHAN_FM4: channel = ECHO_FM4; break;
      case CHAN_FM5: channel = ECHO_FM5; break;
      case CHAN_FM6: channel = ECHO_FM6; break;
      case CHAN_PSG1: channel = ECHO_PSG1; break;
      case CHAN_PSG2: channel = ECHO_PSG2; break;
      case CHAN_PSG3: channel = ECHO_PSG3; break;
      case CHAN_PSG4: channel = ECHO_PSG4; break;
      case CHAN_PSG4EX: channel = ECHO_PSG4; break;
      case CHAN_PCM: return ERR_NONE;

      case CHAN_NONE: return ERR_NONE;
      default: return ERR_UNKNOWN; break;
   }

   // Write instrument change event
   if (fputc(ECHO_INSTR | channel, file) == EOF)
      return ERR_WRITEESF;
   if (fputc(instrument, file) == EOF)
      return ERR_WRITEESF;

   // Quick hack to ensure PSG3 has a valid instrument in PSG3+PSG4 mode
   // Dirty, but it's probably better than spamming set frequency events
   // for PSG3 (as those take up more space than note on events).
   if (channel == CHAN_PSG4EX) {
      if (fputc(ECHO_INSTR | ECHO_PSG3, file) == EOF)
         return ERR_WRITEESF;
      if (fputc(instrument, file) == EOF)
         return ERR_WRITEESF;
   }

   // Success!
   return ERR_NONE;
}

//***************************************************************************
// write_volume [internal]
// Writes the event to change a channel volume.
//---------------------------------------------------------------------------
// param file: output file
// param channel: affected channel
// param volume: new volume
// return: error code
//***************************************************************************

static int write_volume(FILE *file, int channel, int volume) {
   // FM channel?
   if (channel >= CHAN_FM1 && channel <= CHAN_FM6) {
      // Determine output Echo channel
      uint8_t out_chan;
      switch (channel) {
         case CHAN_FM1: out_chan = ECHO_FM1; break;
         case CHAN_FM2: out_chan = ECHO_FM2; break;
         case CHAN_FM3: out_chan = ECHO_FM3; break;
         case CHAN_FM4: out_chan = ECHO_FM4; break;
         case CHAN_FM5: out_chan = ECHO_FM5; break;
         case CHAN_FM6: out_chan = ECHO_FM6; break;
         default: return ERR_UNKNOWN;
      }

      // Output event values
      if (fputc(ECHO_VOLUME | out_chan, file) == EOF)
         return ERR_WRITEESF;
      if (fputc(volume_fm[volume], file) == EOF)
         return ERR_WRITEESF;
   }

   // PSG channel?
   else if (channel >= CHAN_PSG1 && channel <= CHAN_PSG4EX) {
      // Determine output Echo channel
      uint8_t out_chan;
      switch (channel) {
         case CHAN_PSG1: out_chan = ECHO_PSG1; break;
         case CHAN_PSG2: out_chan = ECHO_PSG2; break;
         case CHAN_PSG3: out_chan = ECHO_PSG3; break;
         case CHAN_PSG4: out_chan = ECHO_PSG4; break;
         case CHAN_PSG4EX: out_chan = ECHO_PSG4; break;
         default: return ERR_UNKNOWN;
      }

      // Output event values
      if (fputc(ECHO_VOLUME | out_chan, file) == EOF)
         return ERR_WRITEESF;
      if (fputc(volume_psg[volume], file) == EOF)
         return ERR_WRITEESF;
   }

   // Success!
   return ERR_NONE;
}

//***************************************************************************
// write_panning [internal]
// Writes an event to pan a channel.
//---------------------------------------------------------------------------
// param file: output file
// param channel: affected channel
// param panning: 0x80 = left, 0xC0 = centered, 0x40 = right
// return: error code
//***************************************************************************

static int write_panning(FILE *file, int channel, int panning) {
   // Determine which Echo channel gets the panning
   switch (channel) {
      case CHAN_FM1: channel = ECHO_FM1; break;
      case CHAN_FM2: channel = ECHO_FM2; break;
      case CHAN_FM3: channel = ECHO_FM3; break;
      case CHAN_FM4: channel = ECHO_FM4; break;
      case CHAN_FM5: channel = ECHO_FM5; break;
      case CHAN_FM6: channel = ECHO_FM6; break;
      case CHAN_PSG1: return ERR_NONE;
      case CHAN_PSG2: return ERR_NONE;
      case CHAN_PSG3: return ERR_NONE;
      case CHAN_PSG4: return ERR_NONE;
      case CHAN_PSG4EX: return ERR_NONE;
      case CHAN_PCM: channel = ECHO_FM6; break;

      case CHAN_NONE: return ERR_NONE;
      default: return ERR_UNKNOWN; break;
   }

   // Write panning event
   if (fputc(ECHO_PAN | channel, file) == EOF)
      return ERR_WRITEESF;
   if (fputc(panning, file) == EOF)
      return ERR_WRITEESF;

   // Success!
   return ERR_NONE;
}

//***************************************************************************
// write_delay [internal]
// Writes events to generate a delay in a ESF stream.
//---------------------------------------------------------------------------
// param file: output file
// param amount: frames to wait
// return: error code
//***************************************************************************

static int write_delay(FILE *file, uint64_t amount) {
   // To store the bytes we'll write
   uint8_t buffer[2] = { ECHO_DELAY, 0x00 };

   // If the delay is too big to fit within a single event, split it into
   // multiple delays
   while (amount > 0x100) {
      amount -= 0x100;
      if (fwrite(buffer, 1, 2, file) < 2)
         return ERR_WRITEESF;
   }

   // Write remaining amount of delay
   buffer[1] = amount;
   if (fwrite(buffer, 1, 2, file) < 2)
      return ERR_WRITEESF;

   // Success!
   return ERR_NONE;
}

//***************************************************************************
// write_loopstart [internal]
// Writes the event to indicate the start of a loop in a stream.
//---------------------------------------------------------------------------
// param file: output file
// return: error code
//***************************************************************************

static int write_loopstart(FILE *file) {
   // Write loop start event
   if (fputc(ECHO_LOOPSTART, file) == EOF)
      return ERR_WRITEESF;

   // Success!
   return ERR_NONE;
}

//***************************************************************************
// write_loopend [internal]
// Writes the event to end a looping stream.
//---------------------------------------------------------------------------
// param file: output file
// return: error code
//***************************************************************************

static int write_loopend(FILE *file) {
   // Write loop end event
   if (fputc(ECHO_LOOPEND, file) == EOF)
      return ERR_WRITEESF;

   // Success!
   return ERR_NONE;
}

//***************************************************************************
// write_end [internal]
// Writes the event to end a non-looping stream.
//---------------------------------------------------------------------------
// param file: output file
// return: error code
//***************************************************************************

static int write_end(FILE *file) {
   // Write end-of-stream event
   if (fputc(ECHO_STOP, file) == EOF)
      return ERR_WRITEESF;

   // Success!
   return ERR_NONE;
}
