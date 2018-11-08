// Required headers
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "stream.h"

// Channel types
// High byte of a channel group determines what kind of channels are affected
// (low byte is a bitfield indicating every channel), only one channel group
// can be set at once (otherwise it's an error)
#define CHAN_FM      0x0100   // Echo channels FM1~FM6
#define CHAN_PSG     0x0200   // Echo channels PSG1~PSG4
#define CHAN_PCM     0x0400   // Echo channels PCM
#define CHAN_CTRL    0x0800   // Control channel (for global commands)

// Pointer to every macro
// Uppercase letters go from 0 to 25
// Lowercase letters go from 26 to 51
#define MAX_MACROS 52
static char *macros[MAX_MACROS] = { NULL };

// Status of every channel
#define NUM_CHAN (0x10+1)
static struct {
   uint64_t timestamp;     // Current timestamp
   int octave;             // Current octave
   int transpose;          // Current transpose
   int volume;             // Current volume
   unsigned length;        // Default length
   unsigned instrument;    // Current instrument
   int nullify;            // Nullify next note on/off
   int slide;              // Treat next note as a slide
} chanstat[NUM_CHAN];

// Frequencies for each semitone
// Used for slide notes
static const unsigned fm_freq[] = {
   644, 681, 722, 765,
   810, 858, 910, 964,
   1021,1081,1146,1214
};
static const unsigned psg_freq[] = {
   851, 803, 758, 715,
   675, 637, 601, 568,
   536, 506, 477, 450
};

// Private function prototypes
static char *read_line(FILE *);
static const char *skip_whitespace(const char *);
static const char *skip_no_whitespace(const char *);
static int parse_number(const char **);
static int to_macro_id(char);
static const char *get_macro(char, unsigned);
static int set_macro(char, const char *, unsigned);
static char *expand_macros(const char *, unsigned);
static void free_macros(void);
static uint16_t get_channels(const char *, unsigned);
static int parse_commands(const char *, unsigned, unsigned);
static int parse_length(const char **, unsigned);

//***************************************************************************
// Some replacements for ctype functions
// Doing this because on certain platforms *coughwindowscough* the ctype
// functions can misbehave when the character isn't ASCII. (this had bitten
// me back when trying to figure out a parsing error in Sol)
//***************************************************************************

static inline int is_whitespace(char c) {
   return c == ' ' || (c >= 0x09 && c <= 0x0D);
}

static inline int is_number(char c) {
   return c >= '0' && c <= '9';
}

//***************************************************************************
// parse_mml
// Parses the MML file.
//---------------------------------------------------------------------------
// param filename: filename of PMD file
// return: 0 on success, -1 on failure
//***************************************************************************

int parse_mml(const char *filename)
{
   // Open MML file
   FILE *file = fopen(filename, "r");
   if (file == NULL) {
      fprintf(stderr, "Error: couldn't open MML file \"%s\"\n", filename);
      return -1;
   }

   // Initialize all channels
   for (int i = 0; i < NUM_CHAN; i++) {
      chanstat[i].timestamp = 0;
      chanstat[i].octave = 3;
      chanstat[i].transpose = 0;
      chanstat[i].volume = 15;
      chanstat[i].length = 0x80;
      chanstat[i].instrument = 0;
      chanstat[i].nullify = 0;
      chanstat[i].slide = 0;
   }

   // Go through all lines
   unsigned line_num = 1;
   for (;;) {
      // Get next line
      char *line = read_line(file);
      if (line == NULL)
         break;

      // Skip initial whitespace
      const char *ptr = skip_whitespace(line);
      if (*ptr == '\0') {
         free(line);
         line_num++;
         continue;
      }

      // Comment?
      if (*ptr == ';') {
         free(line);
         line_num++;
         continue;
      }

      // We don't process any hash command yet
      // So same as comments for now but that may change
      if (*ptr == '#') {
         free(line);
         line_num++;
         continue;
      }

      // Set a macro?
      if (*ptr == '!') {
         // Get macro name
         char name = ptr[1];
         ptr += 2;

         // Get to the macro definition
         ptr = skip_whitespace(ptr);
         if (*ptr == '\0') {
            fprintf(stderr, "Error [%u]: missing macro definition\n",
               line_num);
            free(line);
            goto error;
         }

         // Define the macro
         char *expanded = expand_macros(ptr, line_num);
         if (expanded == NULL) {
            free(line);
            goto error;
         }

         if (set_macro(name, expanded, line_num)) {
            free(expanded);
            free(line);
            goto error;
         }

         // Clean up
         free(expanded);
      }

      // Commands for a channel?
      else {
         // Retrieve the channels
         uint16_t channels = get_channels(ptr, line_num);
         if (channels == 0) {
            free(line);
            goto error;
         }
         ptr = skip_no_whitespace(ptr);
         ptr = skip_whitespace(ptr);
         if (*ptr == '\0') {
            free(line);
            line_num++;
            continue;
         }

         // Expand the macros as needed
         char *expanded = expand_macros(ptr, line_num);
         if (expanded == NULL) {
            free(line);
            goto error;
         }

         // Process every channel
         unsigned basechan;
         switch (channels & 0xFF00) {
            case CHAN_FM: basechan = 0x00; break;
            case CHAN_PSG: basechan = 0x08; break;
            case CHAN_PCM: basechan = 0x0C; break;
            case CHAN_CTRL: basechan = 0x10; break;
            default: abort();
         }

         for (unsigned bit = 0; bit < 8; bit++) {
            if (!(channels & 1 << bit))
               continue;

            unsigned chanid = basechan + bit;
            if (parse_commands(expanded, chanid, line_num)) {
               free(expanded);
               free(line);
               goto error;
            }
         }

         // Clean up
         free(expanded);
      }

      // Done with the line
      free(line);
      line_num++;
   }

   // Done
   free_macros();
   fclose(file);
   return 0;

   // Uh oh
error:
   free_macros();
   fclose(file);
   return -1;
}

//***************************************************************************
// read_line [internal]
// Reads a line from a file. This will also filter out comments if found.
//---------------------------------------------------------------------------
// param file: file handle
// return: pointer to line (NULL if end of file)
//---------------------------------------------------------------------------
// notes: call free once you're done with the line
//***************************************************************************

static char *read_line(FILE *file)
{
   // End of file already?
   if (feof(file))
      return NULL;

   // Allocate initial buffer
   size_t bufpos = 0;
   size_t bufsize = 0x80;
   char *buffer = (char*) malloc(bufsize + 1);
   if (buffer == NULL) {
      fputs("Error: out of memory\n", stderr);
      exit(EXIT_FAILURE);
   }

   // Read until the end of the line
   for (;;) {
      // Get next character
      int ch = fgetc(file);
      if (ch == EOF) break;
      if (ch == '\n') break;
      if (ch == '\r') break;
      if (ch == '\0') ch = ' ';
      if (ch == ';') ch = '\0';

      // Put it in the buffer
      buffer[bufpos] = ch;
      bufpos++;

      if (bufpos == bufsize) {
         bufsize += 0x20;
         char *temp = (char*) realloc(buffer, bufsize);
         if (temp == NULL) {
            fputs("Error: out of memory\n", stderr);
            exit(EXIT_FAILURE);
         } else {
            buffer = temp;
         }
      }
   }

   // Done
   buffer[bufpos] = '\0';
   return buffer;
}

//***************************************************************************
// skip_whitespace [internal]
// Skips to the first non-whitespace character in the string, then returns
// a pointer there. (if there aren't any more, it just returns a pointer to
// the terminating nul)
//---------------------------------------------------------------------------
// param ptr: original location
// return: first non-whitespace location
//***************************************************************************

static const char *skip_whitespace(const char *ptr)
{
   while (is_whitespace(*ptr)) ptr++;
   return ptr;
}

//***************************************************************************
// skip_no_whitespace [internal]
// The opposite of skip_whitespace.
//---------------------------------------------------------------------------
// param ptr: original location
// return: first whitespace location
//***************************************************************************

static const char *skip_no_whitespace(const char *ptr)
{
   while (!is_whitespace(*ptr)) ptr++;
   return ptr;
}

//***************************************************************************
// parse_number [internal]
// Parses a number in a string. Returns the value there, or -1 if there isn't
// a number. The pointer is updated based on how many characters it read.
//---------------------------------------------------------------------------
// param ptrptr: pointer to pointer to string
// return: numeric value (-1 if none)
//***************************************************************************

static int parse_number(const char **ptrptr)
{
   // Get actual pointer
   const char *ptr = *ptrptr;

   // No number here?
   if (!is_number(*ptr))
      return -1;

   // Parse the number here
   int value = 0;
   while (is_number(*ptr)) {
      value = value * 10 + (*ptr - '0');
      ptr++;
   }

   // Done
   *ptrptr = ptr;
   return value;
}

//***************************************************************************
// to_macro_id [internal]
// Figures out the internal ID for a macro.
//---------------------------------------------------------------------------
// param name: macro name (a letter)
// return: macro ID (-1 if invalid)
//***************************************************************************

static int to_macro_id(char name)
{
   if (name >= 'A' && name <= 'Z') return name - 'A';
   if (name >= 'a' && name <= 'z') return name - 'a' + 26;
   return -1;
}

//***************************************************************************
// get_macro [internal]
// Retrieves the contents of a macro.
//---------------------------------------------------------------------------
// param name: macro name (a letter)
// param line: line being parsed
// return: macro definition (NULL on failure)
//***************************************************************************

static const char *get_macro(char name, unsigned line)
{
   // Figure out macro ID
   // Also rule out invalid names
   int id = to_macro_id(name);
   if (id == -1) {
      fprintf(stderr, "Error [%u]: \"!%c\" is not a valid macro name\n",
         line, name);
      return NULL;
   }

   // Is the macro defined?
   if (macros[id] == NULL) {
      fprintf(stderr, "Error [%u]: macro \"!%c\" is not defined\n",
         line, name);
      return NULL;
   }

   // Got it
   return macros[id];
}

//***************************************************************************
// set_macro [internal]
// Defines a macro.
//---------------------------------------------------------------------------
// param name: macro name (a letter)
// param data: macro definition
// param line: line being parsed
// return: 0 on success, -1 on failure
//***************************************************************************

static int set_macro(char name, const char *data, unsigned line)
{
   // Figure out macro ID
   // Also rule out invalid names
   int id = to_macro_id(name);
   if (id == -1) {
      fprintf(stderr, "Error [%u]: \"!%c\" is not a valid macro name\n",
         line, name);
      return -1;
   }

   // Was the macro already defined?
   if (macros[id] != NULL) {
      free(macros[id]);
      macros[id] = NULL;
   }

   // Store the macro
   macros[id] = (char*) malloc(strlen(data)+1);
   if (macros[id] == NULL) {
      fputs("Error: out of memory\n", stderr);
      exit(EXIT_FAILURE);
   }

   strcpy(macros[id], data);

#ifdef DEBUG
   fprintf(stderr, "Debug [%u]: Defined macro \"!%c\" as \"%s\"\n",
      line, name, data);
#endif

   // Done
   return 0;
}

//***************************************************************************
// expand_macros [internal]
// Expands all the macros in the string so they can be processed
//---------------------------------------------------------------------------
// param text: original string
// param line: line being parsed
// return: expanded string (NULL on failure)
//---------------------------------------------------------------------------
// notes: call free when you're done with the string
//***************************************************************************

static char *expand_macros(const char *text, unsigned line)
{
   // Determine how much space do we need
   // Filter out invalid macros while we're at it
   size_t len = 0;
   for (const char *ptr = text; *ptr != '\0'; ptr++) {
      // Not a macro, leave as-is
      if (*ptr != '!') {
         len++;
      }

      // Expand macro then
      else {
         ptr++;
         const char *replacement = get_macro(*ptr, line);
         if (replacement == NULL)
            return NULL;
         len += strlen(replacement);
      }
   }

   // Allocate buffer for the expanded string
   char *buffer = (char*) malloc(len + 1);
   if (buffer == NULL) {
      fputs("Error: out of memory\n", stderr);
      exit(EXIT_FAILURE);
   }

   // NOW proceed to do the expansion
   // This time we don't check for invalid macros because we did that earlier
   // so we know they're all valid
   char *dest = buffer;
   for (const char *src = text; *src != '\0'; src++) {
      // Not a macro, leave as-is
      if (*src != '!') {
         *dest = *src;
         dest++;
      }

      // Expand macro then
      else {
         src++;
         const char *replacement = get_macro(*src, line);
         strcpy(dest, replacement);
         dest += strlen(replacement);
      }
   }

   // Done
   *dest = '\0';
   return buffer;
}

//***************************************************************************
// free_macros [internal]
// Purges all macros from memory.
//***************************************************************************

static void free_macros(void)
{
   // Deallocate any macros
   for (size_t i = 0; i < MAX_MACROS; i++) {
      free(macros[i]);
      macros[i] = NULL;
   }
}

//***************************************************************************
// get_channels [internal]
// Parses the letter group to check what channels are specified.
//---------------------------------------------------------------------------
// param ptr: string to check
// param line: line number
// return: channel bitfield (0 on failure)
//***************************************************************************

static uint16_t get_channels(const char *ptr, unsigned line)
{
   // Scan every letter to see what channels are there
   uint16_t channels = 0;

   for (; !is_whitespace(*ptr); ptr++)
   switch (*ptr) {
      // FM channels
      case 'A': channels |= CHAN_FM | 0x01; break;
      case 'B': channels |= CHAN_FM | 0x02; break;
      case 'C': channels |= CHAN_FM | 0x04; break;
      case 'D': channels |= CHAN_FM | 0x10; break;
      case 'E': channels |= CHAN_FM | 0x20; break;
      case 'F': channels |= CHAN_FM | 0x40; break;

      // PSG channels
      case 'G': channels |= CHAN_PSG | 0x01; break;
      case 'H': channels |= CHAN_PSG | 0x02; break;
      case 'I': channels |= CHAN_PSG | 0x04; break;
      case 'J': channels |= CHAN_PSG | 0x08; break;

      // PCM channel
      case 'K': channels |= CHAN_PCM | 0x01; break;

      // Control channel
      case 'Z': channels |= CHAN_CTRL | 0x01; break;

      // Not a valid channel...
      default:
         fprintf(stderr, "Error [%u]: \"%c\" is not a valid channel\n",
            line, *ptr);
         return 0;
   }

   // Ensure there's only one channel type
   uint16_t types = channels & 0xFF00;
   if (types != CHAN_FM && types != CHAN_PSG && types != CHAN_PCM &&
   types != CHAN_CTRL) {
      fprintf(stderr, "Error [%u]: all channels must be the same type\n",
         line);
      return 0;
   }

   // Done
   return channels;
}

//***************************************************************************
// parse_commands [internal]
// Parses a list of MML commands. The core of the language!
//---------------------------------------------------------------------------
// param data: list of commands
// param channel: channel
// param line: line being parsed
// return: 0 on success, -1 on failure
//***************************************************************************

static int parse_commands(const char *data, unsigned channel, unsigned line)
{
   // Process every command
   while (*data != '\0') {
      // Note on?
      if (*data >= 'a' && *data <= 'g') {
         // Control channel shouldn't get note on commands...
         if (channel == 0x10) goto noctrl;

         // Noise channel can't use this reliably, so complain about it
         if (channel == 0x0B) {
            fprintf(stderr,
               "Error[%u]: noise channel can't use normal note on "
               "(use \"n\" command instead)\n", line);
            return -1;
         }

         // Determine base semitone
         int semitone;
         switch (*data) {
            case 'c': semitone = 0; break;
            case 'd': semitone = 2; break;
            case 'e': semitone = 4; break;
            case 'f': semitone = 5; break;
            case 'g': semitone = 7; break;
            case 'a': semitone = 9; break;
            case 'b': semitone = 11; break;
         }
         data++;

         // Sharps and flats
         while (*data == '+' || *data == '-') {
            if (*data == '+') semitone++;
            if (*data == '-') semitone--;
            data++;
         }

         // Adjust to the current octave
         semitone += chanstat[channel].octave * 12;
         // ...and transposing
         semitone += chanstat[channel].transpose;

         // Normalize semitone range
         // <Sik> Leaving the check against zero here for the sake of
         // clarity (FM range is 0x00~0x07) even though it'll be always true
         // since the type is unsigned)
         if (/*channel >= 0x00 &&*/ channel <= 0x07) {
            if (semitone < 0) semitone = 0;
            if (semitone > 95) semitone = 95;
         }
         if (channel >= 0x08 && channel <= 0x0B) {
            semitone -= 24;
            if (semitone < 0) semitone = 0;
            if (semitone > 71) semitone = 71;
         }

         // Parse length if present
         int length = parse_length(&data, line);
         if (length == -1)
            return -1;
         if (length == 0)
            length = chanstat[channel].length;

         // If the note is meant to be a slide we need
         // to use raw frequency values instead
         int slide = chanstat[channel].slide;

         // Add event
         if (!chanstat[channel].nullify) {
            if (!slide) {
               add_note_on(chanstat[channel].timestamp, channel,
               channel != 0x0C ? (unsigned) semitone :
               chanstat[channel].instrument);
            } else {
               add_set_note(chanstat[channel].timestamp, channel,
               semitone);
            }
         }
         chanstat[channel].timestamp += length;
         chanstat[channel].nullify = 0;
         chanstat[channel].slide = 0;
      }

      // Direct note?
      else if (*data == 'n') {
         // Control channel shouldn't get note on commands...
         if (channel == 0x10) goto noctrl;
         data++;

         // Determine note's value to play
         int value = parse_number(&data);
         if (value == -1) {
            fprintf(stderr, "Error[%u]: missing direct note value\n", line);
            return -1;
         }

         // Adjust to the current transpose
         value += chanstat[channel].transpose;

         // Determine note's length, if any
         int length = 0;
         if (*data == ',') {
            data++;
            int length = parse_length(&data, line);
            if (length == -1)
               return -1;
         }
         if (length == 0) {
            length = chanstat[channel].length;
         }

         // Ensure the value is correct for this channel
         int valid = 0;
         if (/*channel >= 0x00 &&*/ channel <= 0x07)
            valid = (value >= 0 && value <= 95);
         else if (channel >= 0x08 && channel <= 0x0A)
            valid = (value >= 0 && value <= 71);
         else if (channel == 0x0B)
            valid = (value >= 0 && value <= 7);
         else if (channel == 0x0C)
            valid = (value >= 0 && value <= 0xFF);

         if (!valid) {
            fprintf(stderr, "Error[%u]: invalid direct note value \"%d\" "
                            "for this channel\n", line, value);
            return -1;
         }

         // If the note is meant to be a slide we need
         // to use raw frequency values instead
         int slide = chanstat[channel].slide;

         // Add event
         if (!chanstat[channel].nullify) {
            if (!slide) {
               add_note_on(chanstat[channel].timestamp, channel, value);
            } else {
               add_set_note(chanstat[channel].timestamp, channel, value);
            }
         }
         chanstat[channel].timestamp += length;
         chanstat[channel].nullify = 0;
         chanstat[channel].slide = 0;
      }

      // Note off?
      else if (*data == 'r') {
         data++;

         // Parse length if present
         int length = parse_length(&data, line);
         if (length == -1)
            return -1;
         if (length == 0)
            length = chanstat[channel].length;

         // Add event
         if (!chanstat[channel].nullify && channel != 0x10)
            add_note_off(chanstat[channel].timestamp, channel);
         chanstat[channel].timestamp += length;
         chanstat[channel].nullify = 0;
         chanstat[channel].slide = 0;
      }

      // Space? (similar to &r)
      else if (*data == 's') {
         data++;

         // Parse length if present
         int length = parse_length(&data, line);
         if (length == -1)
            return -1;
         if (length == 0)
            length = chanstat[channel].length;

         // Update timestamp
         chanstat[channel].timestamp += length;
         chanstat[channel].nullify = 0;
         chanstat[channel].slide = 0;
      }

      // Ignore next note on/off? (used for mid-note changes)
      else if (*data == '&') {
         chanstat[channel].nullify = 1;
         data++;
      }

      // Treat next note as a slide?
      else if (*data == '_') {
         chanstat[channel].slide = 1;
         data++;
      }

      // Go an octave up or down?
      else if (*data == '>' || *data == '<') {
         // Control channel shouldn't get octave commands...
         if (channel == 0x10) goto noctrl;

         // There we go
         if (*data == '>') chanstat[channel].octave++;
         if (*data == '<') chanstat[channel].octave--;
         data++;
      }

      // Set octave directly?
      else if (*data == 'o') {
         // Control channel shouldn't get octave commands...
         if (channel == 0x10) goto noctrl;
         data++;

         // Get octave
         int octave = parse_number(&data);
         if (octave == -1) {
            fprintf(stderr, "Error[%u]: missing octave number\n", line);
            return -1;
         }
         if (octave < 0 || octave > 7) {
            fprintf(stderr, "Error[%u]: invalid octave number\n", line);
            return -1;
         }

         // Change octave
         chanstat[channel].octave = octave;
      }

      // Set transpose?
      else if (*data == 'K' || *data == 'k') {
         // Control channel shouldn't get transpose commands...
         if (channel == 0x10) goto noctrl;
         int relative = (*data == 'k');
         data++;

         // The transpose command *can* accept negative values
         // so we need to check for the sign before parsing
         int sign = 0;
         if (*data == '-') {
            sign = 1;
            data++;
         }

         // Get transpose
         int transpose = parse_number(&data);
         if (transpose == -1) {
            fprintf(stderr, "Error[%u]: missing transpose amount\n", line);
            return -1;
         }

         // Change transpose
         if (relative)
            chanstat[channel].transpose += sign ? -transpose : transpose;
         else
            chanstat[channel].transpose = sign ? -transpose : transpose;
      }

      // Set default length?
      else if (*data == 'l') {
         data++;

         // Get new default length
         int length = parse_length(&data, line);
         if (length == 0)
            fprintf(stderr, "Error[%u]: you must specify a length\n", line);
         if (length <= 0)
            return -1;

         // Set the new default length
         chanstat[channel].length = length;
      }

      // Increment or decrement volume?
      else if (*data == '(' || *data == ')') {
         // Control channel shouldn't get volume commands...
         if (channel == 0x10) goto noctrl;

         // There we go
         if (*data == '(') chanstat[channel].volume--;
         if (*data == ')') chanstat[channel].volume++;
         data++;

         // Clamp to valid range
         if (chanstat[channel].volume < 0)
            chanstat[channel].volume = 0;
         if (chanstat[channel].volume > 15)
            chanstat[channel].volume = 15;

         // Issue command
         if (channel != 0x0C)
            add_set_vol(chanstat[channel].timestamp, channel,
            chanstat[channel].volume);
      }

      // Set volume?
      else if (*data == 'v') {
         // Control channel shouldn't get volume commands...
         if (channel == 0x10) goto noctrl;
         data++;

         // Check if it's relative
         int sign = 0;
         if (*data == '+') {
            sign = 1;
            data++;
         } else if (*data == '-') {
            sign = -1;
            data++;
         }

         // Get volume
         int volume = parse_number(&data);
         if (volume == -1) {
            fprintf(stderr, "Error[%u]: missing new volume\n", line);
            return -1;
         }
         switch (sign) {
            case 0:
               break;
            case 1:
               volume = chanstat[channel].volume + volume;
               break;
            case -1:
               volume = chanstat[channel].volume - volume;
               break;
         }
         if (volume < 0 || volume > 15) {
            fprintf(stderr, "Error[%u]: invalid volume value\n", line);
            return -1;
         }

         // Change volume
         chanstat[channel].volume = volume;
         if (channel != 0x0C)
            add_set_vol(chanstat[channel].timestamp, channel, volume);
      }

      // Set panning?
      else if (*data == 'p') {
         // Control channel shouldn't get panning commands...
         if (channel == 0x10) goto noctrl;
         data++;

         // Get panning
         int pan = parse_number(&data);
         if (pan == -1) {
            fprintf(stderr, "Error[%u]: missing new panning\n", line);
            return -1;
         }
         if (pan < 0 || pan > 3) {
            fprintf(stderr, "Error[%u]: invalid panning value\n", line);
            return -1;
         }

         // Change panning
         if (/*channel >= 0x00 &&*/ channel <= 0x07)
            add_set_pan(chanstat[channel].timestamp, channel, pan);
      }

      // Set instrument?
      else if (*data == '@') {
         // Check for @# command (set flag)
         // Only @ command supported by the Z channel
         if (data[1] == '#') {
            data += 2;

            // Set or clear flags?
            int set = 1;
            if (*data == '-') {
               set = 0;
               data++;
            }

            // Get flags
            int flags = parse_number(&data);
            if (flags == -1) {
               fprintf(stderr, "Error[%u]: missing flags\n", line);
               return -1;
            }
            if (flags < 0x00 || flags > 0xFF) {
               fprintf(stderr, "Error[%u]: \"%d\" doesn't make valid flags\n",
                  line, flags);
               return -1;
            }

            // Store flags command
            add_set_flags(chanstat[channel].timestamp, set, flags);
            continue;
         }

         // Control channel shouldn't get instrument commands...
         if (channel == 0x10) goto noctrl;
         data++;

         // Check for @$ command (channel lock)
         if (*data == '$') {
            data++;
            add_lock(chanstat[channel].timestamp, channel);
            continue;
         }

         // Get instrument number
         int instrument = parse_number(&data);
         if (instrument == -1) {
            fprintf(stderr, "Error[%u]: missing instrument number\n", line);
            return -1;
         }
         if (instrument < 0x00 || instrument >= 0xFF) {
            fprintf(stderr, "Error[%u]: \"%d\" is not a valid instrument\n",
               line, instrument);
            return -1;
         }

         // Issue event
         if (channel != 0x0C)
            add_set_instr(chanstat[channel].timestamp, channel, instrument);
         chanstat[channel].instrument = instrument;
      }

      // Set register directly?
      else if (*data == 'y') {
         data++;

         // Is it a named register?
         int base = -1, offset = -1;
         if (data[0] == 'D' && data[1] == 'M')
            { data += 2; base = 0x30; offset = parse_number(&data); }
         else if (data[0] == 'T' && data[1] == 'L')
            { data += 2; base = 0x40; offset = parse_number(&data); }
         else if (data[0] == 'K' && data[1] == 'A')
            { data += 2; base = 0x50; offset = parse_number(&data); }
         else if (data[0] == 'D' && data[1] == 'R')
            { data += 2; base = 0x60; offset = parse_number(&data); }
         else if (data[0] == 'S' && data[1] == 'R')
            { data += 2; base = 0x70; offset = parse_number(&data); }
         else if (data[0] == 'S' && data[1] == 'L')
            { data += 2; base = 0x80; offset = parse_number(&data); }
         else if (data[0] == 'S' && data[1] == 'E')
            { data += 2; base = 0x90; offset = parse_number(&data); }

         // Determine target register if it was named
         int reg = -1;
         if (base != -1) {
            // We must ensure it's a valid FM channel
            if (channel > 0x07) {
               fprintf(stderr, "Error[%u]: this command only works on FM "
                  "channels\n", line);
               return -1;
            }

            // Check that operator is valid
            if (offset == -1) {
               fprintf(stderr, "Error[%u]: missing operator\n", line);
               return -1;
            }
            if (offset < 0 || offset > 3) {
               fprintf(stderr, "Error[%u]: \"%d\" is not a valid operator\n",
                  line, offset);
               return -1;
            }

            // Determine target register
            reg = base + (offset * 4) + (channel & 0x03) +
                  (channel & 0x04 ? 0x100 : 0);
         }

         // Get raw register number otherwise
         else {
            reg = parse_number(&data);
            if (reg == -1) {
               fprintf(stderr, "Error[%u]: missing register\n", line);
               return -1;
            }
            if (reg < 0x00 || reg > 0x1FF) {
               fprintf(stderr, "Error[%u]: \"%d\" is not a valid register\n",
                  line, reg);
               return -1;
            }
         }

         // Should be a comma here
         if (*data != ',') {
            fprintf(stderr, "Error[%u]: missing register value\n", line);
            return -1;
         }
         data++;

         // Get value
         int value = parse_number(&data);
         if (value == -1) {
            fprintf(stderr, "Error[%u]: missing register value\n", line);
            return -1;
         }
         if (value < 0x00 || value > 0xFF) {
            fprintf(stderr, "Error[%u]: \"%d\" is not a valid value "
               "for a register\n", line, value);
            return -1;
         }

         // Issue event
         add_set_reg(chanstat[channel].timestamp, reg, value);
      }

      // Set loop point?
      else if (*data == 'L') {
         add_loop(chanstat[channel].timestamp);
         data++;
      }

      // Set tempo?
      else if (*data == 't') {
         data++;
         int tempo = parse_number(&data);
         if (tempo == -1) {
            fprintf(stderr, "Error[%u]: missing speed\n", line);
            return -1;
         }
         if (tempo == 0) {
            fprintf(stderr, "Error[%u]: tempo can't be zero!\n", line);
            return -1;
         }
         add_set_tempo(chanstat[channel].timestamp, tempo);
      }

      // Whitespace? (skip it)
      else if (is_whitespace(*data)) {
         data++;
      }

      // Comment?
      else if (*data == ';') {
         break;
      }

      // Unknown or unimplemented command
      else {
         fprintf(stderr, "Error[%u]: invalid command \"%c\"\n", line, *data);
         return -1;
      }
   }

   // Issue a no-op to let the stream now it needs to play at least for
   // this long for this channel to work
   add_nop(chanstat[channel].timestamp);

   // Done
   return 0;

   // For all those commands that don't work from the control channel
noctrl:
   fprintf(stderr, "Error[%u]: you can't use command \"%c\" "
      "from the control channel\n", line, *data);
   return -1;
}

//***************************************************************************
// parse_length [internal]
// Parses a nominal length value in ticks. The pointer is updated to skip
// over the length characters.
//---------------------------------------------------------------------------
// param ptrptr: pointer to pointer to string
// param line: line number
// return: length in ticks (0 if no length, -1 if outright error)
//***************************************************************************

static int parse_length(const char **ptrptr, unsigned line)
{
   // Check if there's a length here
   int value = parse_number(ptrptr);
   if (value == -1) return 0;

   // Ensure the length is valid
   if (value > 128 || (value & (value - 1))) {
      fprintf(stderr, "Error[%u]: \"%d\" is not a valid length\n",
         line, value);
      return -1;
   }

   // OK, determine length in ticks then
   int length = 0x80 / value;

   // Check for dotted lengths!
   if (**ptrptr == '.') {
      length += length / 2;
      (*ptrptr)++;
   }

   // Tie? (add multiple lengths)
   if (**ptrptr == '^') {
      (*ptrptr)++;
      int extra = parse_length(ptrptr, line);
      if (extra <= 0) {
         fprintf(stderr, "Error[%u]: invalid length tie\n", line);
         return -1;
      }
      length += extra;
   }

   // Done
   return length;
}
