// Required headers
#include <stdio.h>
#include <stdlib.h>
#include "stream.h"

// Function prototypes
static int write_one(FILE *, uint8_t);
static int write_two(FILE *, uint8_t, uint8_t);
static int write_three(FILE *, uint8_t, uint8_t, uint8_t);
static int emit_note_on(FILE *, unsigned, unsigned);
static int emit_note_off(FILE *, unsigned);
static int emit_set_note(FILE *, unsigned, unsigned);
static int emit_set_freq(FILE *, unsigned, unsigned);
static int emit_set_volume(FILE *, unsigned, unsigned);
static int emit_set_panning(FILE *, unsigned, unsigned);
static int emit_set_instr(FILE *, unsigned, unsigned);
static int emit_set_reg(FILE *, unsigned, unsigned);
static int emit_set_flags(FILE *, unsigned, unsigned);
static int emit_lock(FILE *, unsigned);
static int emit_loop(FILE *);

//***************************************************************************
// generate_esf
// Generates the ESF file.
//---------------------------------------------------------------------------
// param filename: filename of ESF file
// return: 0 on success, -1 on failure
//***************************************************************************

int generate_esf(const char *filename)
{
   // Create ESF file
   FILE *file = fopen(filename, "wb");
   if (file == NULL) {
      fprintf(stderr, "Error: couldn't create ESF file \"%s\"\n", filename);
      return -1;
   }

   // Sort events by timestamp, just like they'd appear in Echo
   // Makes things a lot easier for us :)
   sort_stream();

   // Used to keep track of how much delay we need to insert
   uint64_t timestamp = 0;

   // Used to keep track of whether the stream loops or not
   int looping = 0;

   // Tempo used to modify the delta values
   // Measured in how many ticks a whole would last
   uint64_t tempo = 0x80;
   uint64_t tempo_error = 0;

   // Scan every event
   size_t num_events = get_num_events();
   for (size_t i = 0; i < num_events; i++) {
      const Event *ev = get_event(i);

      // This shouldn't happen
      if (timestamp > ev->timestamp) {
         fprintf(stderr, "INTERNAL ERROR: timestamps not sorted properly!\n");
         abort();
      }

      // Insert delays as needed
      // Note: handle edge cases?
      uint64_t delta = ev->timestamp - timestamp;
      delta *= tempo;
      tempo_error += delta % 0x80;
      delta = (delta / 0x80) + (tempo_error / 0x80);
      tempo_error %= 0x80;

      if (delta > 0 && delta <= 0x10) {
         if (write_one(file, 0xD0 + delta - 1)) goto error;
      } else {
         while (delta > 0xFF) {
            if (write_two(file, 0xFE, 0xFF)) goto error;
            delta -= 0xFF;
         }
         if (delta > 0) {
            if (write_two(file, 0xFE, delta)) goto error;
         }
      }
      timestamp = ev->timestamp;

      // Parse the event
      switch (ev->type) {
         case EV_NOTEON:
            if (emit_note_on(file, ev->channel, ev->value)) goto error;
            break;
         case EV_NOTEOFF:
            if (emit_note_off(file, ev->channel)) goto error;
            break;
         case EV_SETNOTE:
            if (emit_set_note(file, ev->channel, ev->value)) goto error;
            break;
         case EV_SETFREQ:
            if (emit_set_freq(file, ev->channel, ev->value)) goto error;
            break;
         case EV_SETVOL:
            if (emit_set_volume(file, ev->channel, ev->value)) goto error;
            break;
         case EV_SETPAN:
            if (emit_set_panning(file, ev->channel, ev->value)) goto error;
            break;
         case EV_SETINSTR:
            if (emit_set_instr(file, ev->channel, ev->value)) goto error;
            break;
         case EV_SETREG:
            if (emit_set_reg(file, ev->channel, ev->value)) goto error;
            break;
         case EV_FLAGS:
            if (emit_set_flags(file, ev->channel, ev->value)) goto error;
            break;
         case EV_LOCK:
            if (emit_lock(file, ev->channel)) goto error;
            break;
         case EV_LOOP:
            if (emit_loop(file)) goto error;
            looping = 1;
            break;
         case EV_SETTEMPO:
            tempo = ev->value;
            break;
         default:
            break;
      }
   }

   // Write stream end
   if (write_one(file, looping ? 0xFC : 0xFF))
      goto error;

   // Done
   fclose(file);
   return 0;

   // Uh oh
error:
   fclose(file);
   return -1;
}

//***************************************************************************
// write_one [internal]
// Writes a single byte to a file.
//---------------------------------------------------------------------------
// param file: file handle
// param byte: value to write
// return: 0 on success, -1 on failure
//***************************************************************************

static int write_one(FILE *file, uint8_t byte)
{
   // Try to write the byte
   if (fwrite(&byte, 1, 1, file) < 1) {
      fprintf(stderr, "Error: couldn't write to ESF file!\n");
      return -1;
   }

   // Done
   return 0;
}

//***************************************************************************
// write_two [internal]
// Writes two bytes to a file.
//---------------------------------------------------------------------------
// param file: file handle
// param byte1: first value to write
// param byte2: second value to write
// return: 0 on success, -1 on failure
//***************************************************************************

static int write_two(FILE *file, uint8_t byte1, uint8_t byte2)
{
   if (write_one(file, byte1)) return -1;
   if (write_one(file, byte2)) return -1;
   return 0;
}

//***************************************************************************
// write_three [internal]
// Writes three bytes to a file.
//---------------------------------------------------------------------------
// param file: file handle
// param byte1: first value to write
// param byte2: second value to write
// param byte3: third value to write
// return: 0 on success, -1 on failure
//***************************************************************************

static int write_three(FILE *file, uint8_t byte1, uint8_t byte2, uint8_t byte3)
{
   if (write_one(file, byte1)) return -1;
   if (write_one(file, byte2)) return -1;
   if (write_one(file, byte3)) return -1;
   return 0;
}

//***************************************************************************
// emit_note_on [internal]
// Generates a note on event on the ESF file.
//---------------------------------------------------------------------------
// param file: file handle
// param channel: affected channel
// param value: note to play
// return: 0 on success, -1 on failure
//***************************************************************************

static int emit_note_on(FILE *file, unsigned channel, unsigned value)
{
   // Correct note value as needed by Echo
   if (/*channel >= 0x00 &&*/ channel <= 0x07) {
      unsigned octave = value / 12;
      unsigned semitone = value % 12;
      value = octave << 5 | semitone << 1 | 0x01;
   }
   if (channel >= 0x08 && channel <= 0x0A) {
      value <<= 1;
   }

   // Emit bytes
   return write_two(file, /*0x00 |*/ channel, value);
}

//***************************************************************************
// emit_note_off [internal]
// Generates a note off event on the ESF file.
//---------------------------------------------------------------------------
// param file: file handle
// param channel: affected channel
// return: 0 on success, -1 on failure
//***************************************************************************

static int emit_note_off(FILE *file, unsigned channel)
{
   // Emit byte
   return write_one(file, 0x10 | channel);
}

//***************************************************************************
// emit_set_note [internal]
// Generates a set semitone event on the ESF file.
//---------------------------------------------------------------------------
// param file: file handle
// param channel: affected channel
// param value: note to play
// return: 0 on success, -1 on failure
//***************************************************************************

static int emit_set_note(FILE *file, unsigned channel, unsigned value)
{
   // Correct note value as needed by Echo
   if (/*channel >= 0x00 &&*/ channel <= 0x07) {
      unsigned octave = value / 12;
      unsigned semitone = value % 12;
      value = octave << 4 | semitone;
   }

   // Emit bytes
   return write_two(file, 0x30 | channel, 0x80 | value);
}

//***************************************************************************
// emit_set_freq [internal]
// Generates a set frequency event on the ESF file.
//---------------------------------------------------------------------------
// param file: file handle
// param channel: affected channel
// param value: raw frequency value
// return: 0 on success, -1 on failure
//***************************************************************************

static int emit_set_freq(FILE *file, unsigned channel, unsigned value)
{
   // Emit bytes
   if (channel <= 0x0A)
      return write_three(file, 0x30 | channel, value >> 8, value);
   else
      return write_two(file, 0x30 | channel, value);
}

//***************************************************************************
// emit_set_volume [internal]
// Generates a set volume event on the ESF file.
//---------------------------------------------------------------------------
// param file: file handle
// param channel: affected channel
// param volume: new volume
// return: 0 on success, -1 on failure
//***************************************************************************

static int emit_set_volume(FILE *file, unsigned channel, unsigned volume)
{
   // Emit bytes
   return write_two(file, 0x20 | channel, volume);
}

//***************************************************************************
// emit_set_panning [internal]
// Generates a set panning event on the ESF file.
//---------------------------------------------------------------------------
// param file: file handle
// param channel: affected channel
// param volume: new volume
// return: 0 on success, -1 on failure
//***************************************************************************

static int emit_set_panning(FILE *file, unsigned channel, unsigned panning)
{
   // Emit bytes
   return write_two(file, 0xF0 | channel, panning << 6);
}

//***************************************************************************
// emit_set_instr [internal]
// Generates a set instrument event on the ESF file.
//---------------------------------------------------------------------------
// param file: file handle
// param channel: affected channel
// param instrument: instrument number
// return: 0 on success, -1 on failure
//***************************************************************************

static int emit_set_instr(FILE *file, unsigned channel, unsigned instrument)
{
   // Emit bytes
   return write_two(file, 0x40 | channel, instrument);
}

//***************************************************************************
// emit_set_reg [internal]
// Generates a set register event on the ESF file.
//---------------------------------------------------------------------------
// param file: file handle
// param reg: affected register
// param value: new value
// return: 0 on success, -1 on failure
//***************************************************************************

static int emit_set_reg(FILE *file, unsigned reg, unsigned value)
{
   // Emit bytes
   return write_three(file,
      0xF8 + (reg >> 8), reg & 0xFF, value);
}

//***************************************************************************
// emit_set_flags [internal]
// Generates a set/clear flags event on the ESF file.
//---------------------------------------------------------------------------
// param file: file handle
// param setclr: 1 = set, 0 = clear
// param flags: flags bitfield
// return: 0 on success, -1 on failure
//***************************************************************************

static int emit_set_flags(FILE *file, unsigned setclr, unsigned flags)
{
   // Emit bytes
   return write_two(file, setclr ? 0xFA : 0xFB,
      flags ^ (setclr ? 0x00 : 0xFF));
}

//***************************************************************************
// emit_lock [internal]
// Generates a lock event on the ESF file.
//---------------------------------------------------------------------------
// param file: file handle
// param channel: affected channel
// return: 0 on success, -1 on failure
//***************************************************************************

static int emit_lock(FILE *file, unsigned channel)
{
   // Emit byte
   return write_one(file, 0xE0 | channel);
}

//***************************************************************************
// emit_loop [internal]
// Generates a loop point event on the ESF file.
//---------------------------------------------------------------------------
// param file: file handle
// return: 0 on success, -1 on failure
//***************************************************************************

static int emit_loop(FILE *file)
{
   // Emit byte
   return write_one(file, 0xFD);
}
