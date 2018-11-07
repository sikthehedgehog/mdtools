// Required headers
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "main.h"

// Function prototypes
int load_rom(const char *, Rom *);
int save_rom(const char *, const Rom *);
void pad_rom(Rom *, PadMode, const char *);
void compute_checksum(Rom *);
void fix_build_date(Rom *);
int change_title(const char *, Rom *, const char *);
int change_copyright(const char *, Rom *, const char *);
int change_serial(const char *, Rom *, const char *);
int change_revision(const char *, Rom *, const char *);

//****************************************************************************
// main
// Where the program starts.
//----------------------------------------------------------------------------
// param argc: number of arguments
// param argv: list of arguments
// return: EXIT_SUCCESS if all went OK, EXIT_FAILURE otherwise
//****************************************************************************

int main(int argc, char **argv) {
   const char *filename = NULL;
   const char *title = NULL;
   const char *copyright = NULL;
   const char *serial = NULL;
   const char *revision = NULL;
   int update_date = 0;
   PadMode pad = PADDING_QUIET;

   // Scan every argument
   int options_ok = 1;
   int show_help = 0;
   int show_version = 0;
   int failure = 0;

   for (int i = 1; i < argc; i++) {
      // Option?
      if (options_ok && argv[i][0] == '-') {
         // Stop taking options?
         if (strcmp(argv[i], "--") == 0) {
            options_ok = 0;
         }

         // Show usage or version?
         else if (strcmp(argv[i], "-h") == 0 ||
         strcmp(argv[i], "-?") == 0 ||
         strcmp(argv[i], "--help") == 0) {
            show_help = 1;
         }
         else if (strcmp(argv[i], "-v") == 0 ||
         strcmp(argv[i], "--version") == 0) {
            show_version = 1;
         }

         // Change ROM title?
         else if (strcmp(argv[i], "-t") == 0 ||
         strcmp(argv[i], "--title") == 0) {
            i++;

            if (title != NULL) {
               fprintf(stderr, "Error: there can be only one ROM title\n");
               failure = 1;
               continue;
            }
            if (i == argc) {
               fprintf(stderr, "Error: missing ROM title\n");
               failure = 1;
               continue;
            }

            title = argv[i];
         }

         // Change copyright code?
         else if (strcmp(argv[i], "-c") == 0 ||
         strcmp(argv[i], "--copyright") == 0) {
            i++;

            if (copyright != NULL) {
               fprintf(stderr, "Error: there can be only one copyright code\n");
               failure = 1;
               continue;
            }
            if (i == argc) {
               fprintf(stderr, "Error: missing copyright code\n");
               failure = 1;
               continue;
            }

            copyright = argv[i];
         }

         // Change serial number?
         else if (strcmp(argv[i], "-s") == 0 ||
         strcmp(argv[i], "--serial") == 0) {
            i++;

            if (serial != NULL) {
               fprintf(stderr, "Error: there can be only one serial number\n");
               failure = 1;
               continue;
            }
            if (i == argc) {
               fprintf(stderr, "Error: missing serial number\n");
               failure = 1;
               continue;
            }

            serial = argv[i];
         }

         // Change revision?
         else if (strcmp(argv[i], "-r") == 0 ||
         strcmp(argv[i], "--revision") == 0) {
            i++;

            if (revision != NULL) {
               fprintf(stderr, "Error: there can be only one revision\n");
               failure = 1;
               continue;
            }
            if (i == argc) {
               fprintf(stderr, "Error: missing revision\n");
               failure = 1;
               continue;
            }

            revision = argv[i];
         }

         // Change date?
         else if (strcmp(argv[i], "-d") == 0 ||
         strcmp(argv[i], "--date") == 0) {
            update_date = 1;
         }

         // Verbose padding?
         else if (strcmp(argv[i], "-z") == 0 ||
         strcmp(argv[i], "--size") == 0) {
            pad = PADDING_VERBOSE;
         }

         // Invalid option
         else {
            fprintf(stderr, "Error: unknown option \"%s\"\n", argv[i]);
            failure = 1;
         }
      }

      // ROM filename?
      else if (filename == NULL) {
         filename = argv[i];
      } else {
         fprintf(stderr, "Error: too many filenames\n");
         failure = 1;
      }
   }

   // Can't proceed?
   if (failure) {
      return EXIT_FAILURE;
   }

   // Show usage?
   if (show_help) {
      puts("Usage: romfix [<options>] <filename.bin>\n\n"
           "Options:\n"
           "-- .............. no more options\n"
           "-t <title> ...... set ROM title\n"
           "-c <code> ....... set copyright code\n"
           "-s <serial> ..... set serial number\n"
           "-r <revision> ... set revision (00 to 99)\n"
           "-d .............. set build date to today\n"
           "-z .............. report ROM size before and after padding\n"
           "-h .............. show help\n"
           "-v .............. show version\n");
      return EXIT_SUCCESS;
   }

   // Show version?
   if (show_version) {
      puts("1.0a");
      return EXIT_SUCCESS;
   }

   // Now to get to work
   // ROM must be specified
   if (filename == NULL) {
      fprintf(stderr, "Error: no ROM specified\n");
      return EXIT_FAILURE;
   }

   // Load the ROM
   static Rom rom;
   if (load_rom(filename, &rom)) {
      return EXIT_FAILURE;
   }

   // Do maintenance
   pad_rom(&rom, pad, filename);
   compute_checksum(&rom);

   // Change title?
   if (title != NULL)
   if (change_title(title, &rom, filename)) {
      failure = 1;
   }

   // Change copyright code?
   if (copyright != NULL)
   if (change_copyright(copyright, &rom, filename)) {
      failure = 1;
   }

   // Change serial number?
   if (serial != NULL)
   if (change_serial(serial, &rom, filename)) {
      failure = 1;
   }

   // Change revision?
   if (revision != NULL)
   if (change_revision(revision, &rom, filename)) {
      failure = 1;
   }

   // Update date?
   if (update_date) {
      fix_build_date(&rom);
   }

   // Save ROM back
   if (!failure)
   if (save_rom(filename, &rom)) {
      failure = 1;
   }

   // Done
   return failure ? EXIT_FAILURE : EXIT_SUCCESS;
}

//****************************************************************************
// load_rom
// Loads a ROM into memory.
//----------------------------------------------------------------------------
// param filename: file to load from
// param rom: where to store ROM data
// return: 0 on success, -1 on failure
//****************************************************************************

int load_rom(const char *filename, Rom *rom)
{
   // Try to open the file
   FILE *file = fopen(filename, "rb");
   if (file == NULL) {
      fprintf(stderr, "Error: can't open ROM file \"%s\"\n", filename);
      return -1;
   }

   // Load file into memory
   long size;
   size = fread(rom->blob, 1, MAX_ROM_SIZE, file);
   if (size < 0) {
      fprintf(stderr, "Error: can't read ROM file \"%s\"\n", filename);
      fclose(file);
      return -1;
   }

   // Make sure that ROM isn't too small
   // If it's too small that means the header is invalid and will probably
   // completely wreck the program and I don't want crashes :P
   if (size < MIN_ROM_SIZE) {
      fprintf(stderr, "Error: ROM file \"%s\" is too small\n", filename);
      fclose(file);
      return -1;
   }

   // Make sure that ROM isn't too large
   if (size == MAX_ROM_SIZE) {
      int temp = fgetc(file);
      if (temp != EOF) {
         fprintf(stderr, "Error: ROM file \"%s\" is too large\n", filename);
         fclose(file);
         return -1;
      }
   }
   fclose(file);

   // Odd ROM sizes can cause issues...
   if (size & 1) {
      rom->blob[size] = 0xFF;
      size++;
   }

   // All is OK
   rom->size = size;
   return 0;
}

//****************************************************************************
// save_rom
// Saves the ROM back into its file.
//----------------------------------------------------------------------------
// param filename: file to save into
// param rom: ROM data to save
// return: 0 on success, -1 on failure
//****************************************************************************

int save_rom(const char *filename, const Rom *rom)
{
   // Try to open the file again
   FILE *file = fopen(filename, "wb");
   if (file == NULL) {
      fprintf(stderr, "Error: can't save changes to ROM \"%s\"\n", filename);
      return -1;
   }

   // Write ROM back into the file
   if (fwrite(rom->blob, 1, rom->size, file) != rom->size) {
      fprintf(stderr, "Error: can't write back to ROM \"%s\"\n", filename);
      fclose(file);
      return -1;
   }

   // All is OK
   fclose(file);
   return 0;
}

//****************************************************************************
// pad_rom
// Pads the ROM to the next size we consider safe.
//----------------------------------------------------------------------------
// param rom: pointer to ROM data
// param mode: how to do the padding
// param filename: ROM filename (for reporting)
//****************************************************************************

void pad_rom(Rom *rom, PadMode mode, const char *filename)
{
   // Keep track of old size for reporting later
   size_t old_size = rom->size;

   // Determine what's the next safe size
   size_t x = MIN_ROM_SIZE;
   size_t new_size;

   for (;;) {
      new_size = x;
      if (old_size <= new_size) break;
      new_size = x + (x >> 2);
      if (old_size <= new_size) break;
      new_size = x + (x >> 1);
      if (old_size <= new_size) break;

      x <<= 1;
   }

   // Fill the padding
   rom->size = new_size;
   for (size_t i = old_size; i < new_size; i++)
      rom->blob[i] = 0x00;

   // Report size change?
   if (mode == PADDING_VERBOSE)
      fprintf(stderr, "[%s] old size: %zu bytes, new size: %zu bytes\n",
              filename, old_size, new_size);
}

//****************************************************************************
// compute_checksum
// Computes and updates the checksum for the ROM.
//----------------------------------------------------------------------------
// param rom: pointer to ROM data
//****************************************************************************

void compute_checksum(Rom *rom)
{
   uint16_t sum = 0;

   // Add up all words in the ROM after the header
   uint8_t *ptr = &rom->blob[PROGRAM_START];
   size_t count = (rom->size - PROGRAM_START) / 2;

   while (count > 0) {
      uint16_t word = (ptr[0] << 8) | ptr[1];
      sum += word;
      ptr += 2;
      count--;
   }

   // Write new checksum
   rom->blob[HEADER_CHECKSUM+0] = sum >> 8;
   rom->blob[HEADER_CHECKSUM+1] = sum;
}

//****************************************************************************
// fix_build_date
// Changes the ROM build date to today.
//----------------------------------------------------------------------------
// param rom: pointer to ROM data
//****************************************************************************

void fix_build_date(Rom *rom)
{
   static const char* const months[] = {
      "JAN","FEB","MAR","APR","MAY","JUN",
      "JUL","AUG","SEP","OCT","NOV","DEC"
   };

   // Get current time
   time_t sec_now = time(NULL);
   struct tm *tm_now = localtime(&sec_now);

   // Generate the string
   char buffer[DATE_LEN+1];
   snprintf(buffer, sizeof(buffer), "%04d.%s",
      tm_now->tm_year+1900, months[tm_now->tm_mon]);
   memcpy(&rom->blob[HEADER_DATE], buffer, DATE_LEN);
}

//****************************************************************************
// change_title
// Changes the ROM title fields.
//----------------------------------------------------------------------------
// param title: new title
// param rom: pointer to ROM data
// param filename: ROM filename
// return: 0 on success, -1 on failure
//****************************************************************************

int change_title(const char *title, Rom *rom, const char *filename)
{
   // Make sure that the title is usable
   size_t len = strlen(title);
   if (len > TITLE_LEN) {
      fprintf(stderr, "Error[%s]: ROM title \"%s\" is too large\n",
              filename, title);
      return -1;
   }

   // Copy over the new title
   memset(&rom->blob[HEADER_TITLE1], ' ', TITLE_LEN);
   memcpy(&rom->blob[HEADER_TITLE1], title, len);
   memcpy(&rom->blob[HEADER_TITLE2], &rom->blob[HEADER_TITLE1], TITLE_LEN);

   // All is OK
   return 0;
}

//****************************************************************************
// change_copyright
// Changes the copyright code.
//----------------------------------------------------------------------------
// param copyright: new copyright code
// param rom: pointer to ROM data
// param filename: ROM filename
// return: 0 on success, -1 on failure
//****************************************************************************

int change_copyright(const char *copyright, Rom *rom, const char *filename)
{
   // Make sure that the copyright is usable
   size_t len = strlen(copyright);
   if (len > COPYRIGHT_LEN) {
      fprintf(stderr, "Error[%s]: copyright code \"%s\" is too large\n",
              filename, copyright);
      return -1;
   }

   // Copy over the new copyright code
   memset(&rom->blob[HEADER_COPYRIGHT], ' ', COPYRIGHT_LEN);
   memcpy(&rom->blob[HEADER_COPYRIGHT], copyright, len);

   // All is OK
   return 0;
}

//****************************************************************************
// change_serial
// Changes the serial number.
//----------------------------------------------------------------------------
// param serial: new serial number
// param rom: pointer to ROM data
// param filename: ROM filename
// return: 0 on success, -1 on failure
//****************************************************************************

int change_serial(const char *serial, Rom *rom, const char *filename)
{
   // Make sure that the serial number is usable
   size_t len = strlen(serial);
   if (len > SERIALNO_LEN) {
      fprintf(stderr, "Error[%s]: serial number \"%s\" is too large\n",
              filename, serial);
      return -1;
   }

   // Copy over the new serial number
   memset(&rom->blob[HEADER_SERIALNO], ' ', SERIALNO_LEN);
   memcpy(&rom->blob[HEADER_SERIALNO], serial, len);

   // All is OK
   return 0;
}

//****************************************************************************
// change_revision
// Changes the revision number.
//----------------------------------------------------------------------------
// param revision: new revision
// param rom: pointer to ROM data
// param filename: ROM filename
// return: 0 on success, -1 on failure
//****************************************************************************

int change_revision(const char *revision, Rom *rom, const char *filename)
{
   // Make sure that the revision is usable
   // Shortcircuiting conditions, yay :P
   if (!(revision[0] >= '0' && revision[0] <= '9') ||
       !(revision[1] >= '0' && revision[1] <= '9') ||
       revision[2] != '\0')
   {
      fprintf(stderr, "Error[%s]: revision number \"%s\" is not valid\n",
              filename, revision);
      return -1;
   }

   // Copy over the new revision
   rom->blob[HEADER_REVISION+0] = revision[0];
   rom->blob[HEADER_REVISION+1] = revision[1];

   // All is OK
   return 0;
}
