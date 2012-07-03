//***************************************************************************
// "batch.c"
// Batch file processing
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "main.h"
#include "echo.h"
#include "event.h"
#include "midi.h"

// Structure for storing token information
typedef struct {
   char **tokens;             // Pointer to token list
   unsigned num_tokens;       // Number of tokens
} TokenList;

// Private function prototypes
static int read_line(FILE *, char **);
static int split_tokens(const char *, TokenList *);
static void free_tokens(TokenList *);
static void print_error_line(unsigned);

// Names for each supported channel
static const char *channel_names[] = {
   "fm1", "fm2", "fm3", "fm4", "fm5", "fm6",
   "psg1", "psg2", "psg3", "psg4", "psg3+psg4",
   "pcm"
};

//***************************************************************************
// process_batch
// Processes a batch file.
//---------------------------------------------------------------------------
// param filename: input filename
// return: error code
//***************************************************************************

int process_batch(const char *filename) {
   // To store error codes
   int errcode;

   // If there was a syntax error somewhere, this flag gets set
   // Parsing must continue so all syntax errors can be shown, but no more
   // proper processing will be done at this point
   int failed = 0;

   // Reset all channel mappings
   // All channels are unmapped by default, except MIDI channel 10 which is
   // always mapped to Echo's PCM channel.
   for (unsigned i = 1; i <= NUM_MIDICHAN; i++)
      map_channel(i, i == 10 ? CHAN_PCM : CHAN_NONE);

   // Reset all instrument mappings
   for (unsigned i = 0; i < NUM_MIDIINSTR; i++) {
      map_instrument(INSTR_FM, i, -1, 0, 100);
      map_instrument(INSTR_PSG, i, -1, 0, 100);
      map_instrument(INSTR_PCM, i, -1, 0, 100);
   }

   // Open batch file
   FILE *file = fopen(filename, "r");
   if (file == NULL)
      return ERR_OPENBATCH;

   // Some parameters that affect how the streams are generated
   int looping = 0;

   // Set default range for pitch wheel
   set_pitch_range(2);

   // Read entire file
   for (unsigned line_num = 1; !feof(file); line_num++) {
      // Get next line
      char *line;
      errcode = read_line(file, &line);
      if (errcode) {
         fclose(file);
         return errcode;
      }

      // Get a list of the arguments in this line
      TokenList args;
      errcode = split_tokens(line, &args);
      if (errcode) {
         // Syntax errors?
         if (errcode == ERR_BADQUOTE) {
            print_error_line(line_num);
            fputs("quote inside non-quoted token\n", stderr);
         } else if (errcode == ERR_NOQUOTE) {
            print_error_line(line_num);
            fputs("missing ending quote\n", stderr);
         }

         // Nope, something more serious, give up
         else {
            free(line);
            fclose(file);
            return errcode;
         }

         // Can't continue with this line, move on...
         failed = 1;
         free(line);
         continue;
      }

      // Done with the line
      free(line);

      // No tokens?
      if (args.num_tokens == 0)
         continue;

      // Convert MIDI to ESF?
      if (!strcmp(args.tokens[0], "convert")) {
         // Check number of arguments
         if (args.num_tokens != 3) {
            // Parsing failed...
            failed = 1;

            // Determine message to show depending on how many arguments
            // were we given (determine the cause of the error)
            const char *msg;
            switch (args.num_tokens) {
               case 1: msg = "missing both filenames\n"; break;
               case 2: msg = "missing output filename\n"; break;
               default: msg = "too many arguments\n"; break;
            }

            // Show error message
            print_error_line(line_num);
            fputs(msg, stderr);
         }

         // Read MIDI file
         if (!failed) {
            errcode = read_midi(args.tokens[1]);
            if (errcode) {
               // Conversion failed...
               failed = 1;

               // Determine error message to show based on error code. If
               // it's a serious error (we can't continue) then set the
               // message to NULL so we know to don't handle it.
               const char *msg;
               switch (errcode) {
                  case ERR_OPENMIDI:
                     msg = "couldn't open input file\n"; break;
                  case ERR_READMIDI:
                     msg = "couldn't read from input file\n"; break;
                  case ERR_CORRUPT:
                     msg = "input file isn't a valid MIDI file\n"; break;
                  case ERR_MIDITYPE2:
                     msg = "input file is MIDI type 2 (not supported "
                     "by this tool, sorry)\n"; break;
                  default:
                     msg = NULL; break;
               }

               // FUCK, serious error, bail out
               if (msg == NULL) {
                  free_tokens(&args);
                  fclose(file);
                  return errcode;
               }

               // Show error message
               print_error_line(line_num);
               fputs(msg, stderr);
            }
         }

         // Output ESF file
         if (!failed) {
            errcode = write_esf(args.tokens[2], looping);
            if (errcode) {
               // Conversion failed...
               failed = 1;

               // Determine error message to show based on error code. If
               // it's a serious error (we can't continue) then set the
               // message to NULL so we know to don't handle it.
               const char *msg;
               switch (errcode) {
                  case ERR_OPENESF:
                     msg = "couldn't open output file\n"; break;
                  case ERR_WRITEESF:
                     msg = "couldn't write into output file\n"; break;
                  default:
                     msg = NULL; break;
               }

               // FUCK, serious error, bail out
               if (msg == NULL) {
                  free_tokens(&args);
                  fclose(file);
                  return errcode;
               }

               // Show error message
               print_error_line(line_num);
               fputs(msg, stderr);
            }
         }
      }

      // Channel mapping?
      else if (!strcmp(args.tokens[0], "channel")) {
         // Check number of arguments
         if (args.num_tokens != 3) {
            // Parsing failed...
            failed = 1;

            // Determine message to show depending on how many arguments
            // were we given (determine the cause of the error)
            const char *msg;
            switch (args.num_tokens) {
               case 1: msg = "missing both channels\n"; break;
               case 2: msg = "missing Echo channel\n"; break;
               default: msg = "too many arguments\n"; break;
            }

            // Show error message
            print_error_line(line_num);
            fputs(msg, stderr);
         }

         // Retrieve MIDI channel
         int midi_chan = 0;
         if (args.num_tokens >= 2) {
            midi_chan = atoi(args.tokens[1]);

            // Check that the channel is valid
            if (midi_chan < 1 || midi_chan > 16) {
               failed = 1;
               print_error_line(line_num);
               fprintf(stderr, "\"%s\" is not a valid MIDI channel\n",
                  args.tokens[1]);
            }

            // MIDI channel 10 can't be remapped (it's always the PCM
            // channel since it's hard-coded to percussion in MIDI)
            if (midi_chan == 10) {
               failed = 1;
               print_error_line(line_num);
               fputs("MIDI channel 10 can't be remapped (it's always "
                  "mapped to the Echo PCM channel)\n", stderr);
            }
         }

         // Retrieve target channel
         int out_chan = CHAN_NONE;
         if (args.num_tokens >= 3) {
            // Look if we know this channel name
            for (unsigned i = CHAN_FM1; i <= CHAN_PCM; i++)
               if (!strcmp(args.tokens[2], channel_names[i])) {
                  out_chan = i;
                  break;
               }

            // Nice try...
            if (out_chan == CHAN_PCM) {
               failed = 1;
               if (midi_chan != 10) {
                  print_error_line(line_num);
                  fputs("Can't use the PCM channel with anything other "
                     "than MIDI channel 10\n", stderr);
               }
            }

            // Unknown channel?
            else if (out_chan == CHAN_NONE) {
               failed = 1;
               print_error_line(line_num);
               fprintf(stderr, "\"%s\" is not a valid Echo channel\n",
                  args.tokens[2]);
            }

            // Store channel mapping
            if (!failed)
               map_channel(midi_chan, out_chan);
         }
      }

      // Instrument mapping?
      else if (!strcmp(args.tokens[0], "instrument")) {
         // Optional parameters
         // Use sensible defaults
         int transpose = 0;
         int gain = 100;

         // Check number of arguments
         if (args.num_tokens < 4) {
            // Parsing failed...
            failed = 1;

            // Determine message to show depending on how many arguments
            // were we given (determine the cause of the error)
            const char *msg;
            switch (args.num_tokens) {
               case 1: msg = "missing both instruments and type\n"; break;
               case 2: msg = "missing both instruments\n"; break;
               case 3: msg = "missing Echo instrument\n"; break;
               default: msg = "WTF?\n"; break;
            }

            // Show error message
            print_error_line(line_num);
            fputs(msg, stderr);
         }

         // Determine instrument type
         int instr_type = 0;
         if (args.num_tokens >= 2) {
            if (!strcmp(args.tokens[1], "fm"))
               instr_type = INSTR_FM;
            else if (!strcmp(args.tokens[1], "psg"))
               instr_type = INSTR_PSG;
            else if (!strcmp(args.tokens[1], "pcm"))
               instr_type = INSTR_PCM;
            else {
               failed = 1;
               print_error_line(line_num);
               fprintf(stderr, "\"%s\" is not a valid instrument type.\n",
                  args.tokens[1]);
            }
         }

         // Get MIDI instrument
         int midi_instr = -1;
         if (args.num_tokens >= 3) {
            midi_instr = atoi(args.tokens[2]);
            if (instr_type != INSTR_PCM)
               midi_instr--;
            if (midi_instr < 0 || midi_instr >= NUM_MIDIINSTR) {
               failed = 1;
               print_error_line(line_num);
               fprintf(stderr, "\"%s\" is not a valid MIDI %s.\n",
                  args.tokens[2], instr_type == INSTR_PCM ? "note" :
                  "instrument");
            }
         }

         // Get Echo instrument
         int echo_instr = -1;
         if (args.num_tokens >= 4) {
            echo_instr = atoi(args.tokens[3]);
            if (echo_instr < 0 || echo_instr >= NUM_ECHOINSTR) {
               failed = 1;
               print_error_line(line_num);
               fprintf(stderr, "\"%s\" is not a valid Echo instrument.\n",
                  args.tokens[2]);
            }
         }

         // Look for optional arguments
         for (unsigned i = 4; i < args.num_tokens; ) {
            // Transpose?
            if (!strcmp(args.tokens[i], "transpose")) {
               // Make sure we have the amount of notes to transpose
               if (i + 1 >= args.num_tokens) {
                  failed = 1;
                  fputs("missing semitones to transpose\n", stderr);
                  break;
               }

               // All OK, get amount of semitones to transpose
               transpose = atoi(args.tokens[i+1]);
               i += 2;
            }

            // Volume scaling?
            else if (!strcmp(args.tokens[i], "gain")) {
               // Make sure we have the gain
               if (i + 1 >= args.num_tokens) {
                  failed = 1;
                  fputs("missing gain percentage\n", stderr);
                  break;
               }

               // Get gain and check if it's valid
               gain = atoi(args.tokens[i+1]);
               if (gain < 0) {
                  failed = 1;
                  fputs("gain can't be negative\n", stderr);
               }

               // Keep going
               i += 2;
            }

            // Unknown argument
            else {
               failed = 1;
               fprintf(stderr, "\"%s\" is not a valid optional argument.\n",
                  args.tokens[i]);
               break;
            }
         }

         // Store instrument mapping
         if (!failed) {
            map_instrument(instr_type, midi_instr, echo_instr,
            transpose, gain);
         }
      }

      // Enable/disable looping?
      else if (!strcmp(args.tokens[0], "loop")) {
         // Check number of arguments
         if (args.num_tokens != 2) {
            // Parsing failed...
            failed = 1;

            // Determine message to show depending on how many arguments
            // were we given (determine the cause of the error)
            const char *msg;
            if (args.num_tokens == 1)
               msg = "missing \"on\"/\"off\" argument\n";
            else
               msg = "too many arguments\n";

            // Show error message
            print_error_line(line_num);
            fputs(msg, stderr);
         }

         // Arguments OK, keep going
         else {
            // Determine if to enable or disable looping
            if (!strcmp(args.tokens[1], "on"))
               looping = 1;
            else if (!strcmp(args.tokens[1], "off"))
               looping = 0;

            // Oops?
            else {
               failed = 1;
               print_error_line(line_num);
               fprintf(stderr, "\"%s\" is not a valid looping setting\n",
                  args.tokens[1]);
            }
         }
      }

      // Set the default pitch range?
      else if (!strcmp(args.tokens[0], "pitchrange")) {
         // Check number of arguments
         if (args.num_tokens != 2) {
            // Parsing failed...
            failed = 1;

            // Determine message to show depending on how many arguments
            // were we given (determine the cause of the error)
            const char *msg;
            if (args.num_tokens == 1)
               msg = "missing amount of semitones\n";
            else
               msg = "too many arguments\n";

            // Show error message
            print_error_line(line_num);
            fputs(msg, stderr);
         }

         // Arguments fine, process the new value...
         else {
            // Get range in semitones
            int range = atoi(args.tokens[1]);

            // Check that the range is valid
            if (range <= 0) {
               failed = 1;
               print_error_line(line_num);
               fprintf(stderr, "\"%s\" is not a valid range\n",
                  args.tokens[1]);
            }

            // Set the new range as the default one
            else
               set_pitch_range(range);
         }
      }

      // Reset mappings?
      else if (!strcmp(args.tokens[0], "reset")) {
         // Check number of arguments
         if (args.num_tokens != 2) {
            // Parsing failed...
            failed = 1;

            // Determine message to show depending on how many arguments
            // were we given (determine the cause of the error)
            const char *msg;
            if (args.num_tokens == 1)
               msg = "missing what to reset\n";
            else
               msg = "too many arguments\n";

            // Show error message
            print_error_line(line_num);
            fputs(msg, stderr);
         }

         // Arguments OK, keep going
         else {
            // Reset instruments?
            if (!strcmp(args.tokens[1], "instruments")) {
               for (unsigned i = 0; i < NUM_MIDIINSTR; i++) {
                  map_instrument(INSTR_FM, i, -1, 0, 100);
                  map_instrument(INSTR_PSG, i, -1, 0, 100);
                  map_instrument(INSTR_PCM, i, -1, 0, 100);
               }
            }

            // Reset channels?
            else if (!strcmp(args.tokens[1], "channels")) {
               for (unsigned i = 1; i <= NUM_MIDICHAN; i++)
                  map_channel(i, i == 10 ? CHAN_PCM : CHAN_NONE);
            }

            // What do you want to reset...?
            else {
               failed = 1;
               print_error_line(line_num);
               fprintf(stderr, "\"%s\" is not a valid parameter to reset\n",
                  args.tokens[1]);
            }
         }
      }

      // Unknown command?
      else {
         print_error_line(line_num);
         fprintf(stderr, "unknown command \"%s\"\n", args.tokens[0]);
         failed = 1;
      }

      // Done with the tokens
      free_tokens(&args);
   }

   // Done
   fclose(file);
   return failed ? ERR_PARSE : ERR_NONE;
}

//***************************************************************************
// read_line
// Reads a line from a text file. The pointer to the buffer should be freed
// once it isn't needed anymore.
//---------------------------------------------------------------------------
// param file: input file
// param where: where to store a pointer to the line
// return: error code
//***************************************************************************

#define BUFLINE_SIZE 0x80
#define BUFLINE_STEP 0x20

static int read_line(FILE *file, char **where) {
   // Set the pointer to null for now (if there's an error, this is what the
   // caller will see)
   *where = NULL;

   // Initial size of the buffer
   size_t bufsize = BUFLINE_SIZE;
   size_t bufpos = 0;

   // Allocate initial buffer for the line
   // Allocate one extra char for the terminating null
   char *buffer = (char *) malloc(bufsize+1);
   if (buffer == NULL)
      return ERR_NOMEMORY;

   // Keep reading until the end of the line
   for (;;) {
      // Get next character
      int c = fgetc(file);

      // End of file? Error?
      if (c == EOF) {
         if (feof(file))
            break;
         else {
            free(buffer);
            return ERR_READBATCH;
         }
      }

      // End of line? (take backslash-newline combination into account)
      if (c == '\n' || c == '\r') {
         if (bufpos > 0 && buffer[bufpos-1] == '\\')
            bufpos--;
         else
            break;
      }

      // Null character? (skip!)
      if (c == '\0')
         continue;

      // Put character into the buffer
      buffer[bufpos] = c;
      bufpos++;

      // Do we need a larger buffer?
      if (bufpos == bufsize) {
         bufsize += BUFLINE_STEP;
         char *temp = (char *) malloc(bufsize+1);
         if (temp == NULL) {
            free(buffer);
            return ERR_NOMEMORY;
         } else
            buffer = temp;
      }
   }

   // Success!
   buffer[bufpos] = '\0';
   *where = buffer;
   return ERR_NONE;
}

//***************************************************************************
// split_tokens [internal]
// Parses a line and splits it into tokens (generating a token list).
//---------------------------------------------------------------------------
// param str: line to split into tokens
// param list: where to store token information
// return: error code
//***************************************************************************

static int split_tokens(const char *str, TokenList *list) {
   // Reset list
   list->tokens = NULL;
   list->num_tokens = 0;

   // Go through entire line
   while (*str) {
      // Skip whitespace
      if (isspace(*str)) {
         str++;
         continue;
      }

      // Comment? End of string?
      if (*str == '#' || *str == '\0')
         break;

      // Unquoted token?
      if (*str != '\"') {
         // Make room for a new token in the list
         char **temp_list = (char **) realloc(list->tokens,
            sizeof(char *) * (list->num_tokens + 1));
         if (temp_list == NULL) {
            free_tokens(list);
            return ERR_NOMEMORY;
         } else
            list->tokens = temp_list;

         // Token starts here
         const char *start = str;

         // Try to find the end of the token
         // Also make sure there aren't any quotes inside...
         do {
            str++;
            if (*str == '\"') {
               free_tokens(list);
               return ERR_BADQUOTE;
            }
         } while (!(isspace(*str) || *str == '\0'));

         // Get length of the token
         size_t length = str - start;

         // Allocate memory for this token
         char *buffer = (char *) malloc(length + 1);
         if (buffer == NULL) {
            free_tokens(list);
            return ERR_NOMEMORY;
         } else {
            memcpy(buffer, start, length);
            buffer[length] = '\0';
         }

         // Put token inside the token list
         list->tokens[list->num_tokens] = buffer;
         list->num_tokens++;
      }

      // Quoted token?
      else {
         // Make room for a new token in the list
         char **temp_list = (char **) realloc(list->tokens,
            sizeof(char *) * (list->num_tokens + 1));
         if (temp_list == NULL) {
            free_tokens(list);
            return ERR_NOMEMORY;
         } else
            list->tokens = temp_list;

         // Skip quote
         str++;

         // Mark where the token starts
         // Also we need to count this character by character, since escaped
         // quotes take up two bytes in the string but they will be kept as
         // a single one.
         const char *start = str;
         size_t length = 0;

         // Look for the ending quote
         for (;;) {
            // Uh oh, missing quote...
            if (*str == '\0') {
               free_tokens(list);
               return ERR_NOQUOTE;
            }

            // Quote? Check if it's an escaped quote or the end of the token
            else if (*str == '\"') {
               str++;
               if (*str == '\"') {
                  length++;
                  str++;
               } else
                  break;
            }

            // Normal character, count it as usual
            else {
               length++;
               str++;
            }
         }

         // Allocate memory for this token
         char *buffer = (char *) malloc(length + 1);
         if (buffer == NULL) {
            free_tokens(list);
            return ERR_NOMEMORY;
         }

         // Copy token into buffer, taking into account escaped quotes (which
         // will be stored as a single quote)
         char *ptr = buffer;
         for (size_t i = 0; i < length; i++) {
            if (*start == '\"') {
               *ptr++ = '\"';
               start += 2;
            } else
               *ptr++ = *start++;
         }
         *ptr = '\0';

         // Put token inside the token list
         list->tokens[list->num_tokens] = buffer;
         list->num_tokens++;
      }
   }

   // Success!
   return ERR_NONE;
}

//***************************************************************************
// free_tokens [internal]
// Deallocates the information in a token list.
//---------------------------------------------------------------------------
// param list: pointer to tokens list
//***************************************************************************

static void free_tokens(TokenList *list) {
   // Any tokens?
   if (list->tokens != NULL) {
      // Get rid of the tokens themselves
      for (unsigned i = 0; i < list->num_tokens; i++)
         free(list->tokens[i]);

      // Get rid of pointer list
      free(list->tokens);
   }

   // Reset list
   list->tokens = NULL;
   list->num_tokens = 0;
}

//***************************************************************************
// print_error_line
// Prints the header for syntax error messages
//---------------------------------------------------------------------------
// param line: current line ID
//***************************************************************************

static void print_error_line(unsigned line) {
   fprintf(stderr, "Error [%u]: ", line);
}
