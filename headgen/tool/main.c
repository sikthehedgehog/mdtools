//***************************************************************************
// "main.c"
// Program entry point, parses command line and runs stuff as required
//***************************************************************************
// Mega Drive header generation tool
// Copyright 2011 Javier Degirolmo
//
// This file is part of headgen.
//
// headgen is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// headgen is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with headgen.  If not, see <http://www.gnu.org/licenses/>.
//***************************************************************************

// Required headers
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "main.h"

// Function prototypes
int generate_asm(FILE *, const HeaderInfo *);

//***************************************************************************
// Program entry point
//***************************************************************************

int main(int argc, char **argv) {
   // To know if there was an error or not
   int errcode = 0;

   // Where we store the information to show in the header
   // Initialize it to some sensible default values
   HeaderInfo header;
   header.title[0] = '\0';
   header.copyright[0] = '\0';
   header.pad6 = 0;
   header.mouse = 0;
   header.megacd = 0;
   header.sram = 0;

   // Assume the release date is today by default
   // Why does gmtime take a pointer to time_t when it's an integer?!
   time_t today_secs = time(NULL);
   const struct tm *today_info = gmtime(&today_secs);
   header.year = today_info->tm_year + 1900;
   header.month = today_info->tm_mon;

   // Scan all arguments
   int show_help = 0;
   int show_ver = 0;
   const char *outfilename = NULL;

   int scan_ok = 1;
   int err_manyfiles = 0;

   for (int curr_arg = 1; curr_arg < argc; curr_arg++) {
      // Get pointer to argument, to make our lives easier
      const char *arg = argv[curr_arg];

      // If it's an option, parse it
      if (scan_ok && arg[0] == '-') {
         // Stop parsing options?
         if (!strcmp(arg, "--"))
            scan_ok = 0;

         // Set game title?
         else if (!strcmp(arg, "-t") || !strcmp(arg, "--title")) {
            // Title already specified?
            if (header.title[0] != '\0') {
               fputs("Error: game title already specified\n", stderr);
               errcode = 1;
               continue;
            }

            // Make sure the game title is specified
            curr_arg++;
            if (curr_arg >= argc) {
               fputs("Error: missing game title\n", stderr);
               errcode = 1;
               continue;
            }

            // Ensure the title isn't empty...
            const char *arg = argv[curr_arg];
            if (*arg == '\0') {
               fputs("Error: game title is empty\n", stderr);
               errcode = 1;
               continue;
            }

            // Store game title in the header
            strncpy(header.title, arg, MAX_TITLE);
            header.title[MAX_TITLE] = '\0';
            for (unsigned i = 0; i < MAX_TITLE; i++)
               header.title[i] = toupper(header.title[i]);
         }

         // Set copyright code?
         else if (!strcmp(arg, "-c") || !strcmp(arg, "--copyright")) {
            // Title already specified?
            if (header.copyright[0] != '\0') {
               fputs("Error: copyright code already specified\n", stderr);
               errcode = 1;
               continue;
            }

            // Make sure the copyright code is specified
            curr_arg++;
            if (curr_arg >= argc) {
               fputs("Error: missing copyright code\n", stderr);
               errcode = 1;
               continue;
            }

            // Ensure the copyright code isn't empty...
            const char *arg = argv[curr_arg];
            if (*arg == '\0') {
               fputs("Error: copyright code is empty\n", stderr);
               errcode = 1;
               continue;
            }

            // Store copyright code in the header
            strncpy(header.copyright, arg, MAX_COPYRIGHT);
            header.copyright[MAX_COPYRIGHT] = '\0';
            for (unsigned i = 0; i < MAX_COPYRIGHT; i++)
               header.copyright[i] = toupper(header.copyright[i]);
         }

         // Functionality support?
         else if (!strcmp(arg, "-6") || !strcmp(arg, "--6pad"))
            header.pad6 = 1;
         else if (!strcmp(arg, "-m") || !strcmp(arg, "--mouse"))
            header.mouse = 1;
         else if (!strcmp(arg, "-cd") || !strcmp(arg, "--megacd"))
            header.megacd = 1;
         else if (!strcmp(arg, "-s") || !strcmp(arg, "--sram"))
            header.sram = 1;

         // Show help or version?
         else if (!strcmp(arg, "-h") || !strcmp(arg, "--help"))
            show_help = 1;
         else if (!strcmp(arg, "-v") || !strcmp(arg, "--version"))
            show_ver = 1;

         // Unknown argument
         else {
            fprintf(stderr, "Error: unknown option \"%s\"\n", arg);
            errcode = 1;
         }
      }

      // Output filename?
      else if (outfilename == NULL)
         outfilename = arg;

      // Too many files specified?
      else
         err_manyfiles = 1;
   }

   // Look for error conditions
   if (!show_help && !show_ver && err_manyfiles) {
      errcode = 1;
      fputs("Error: too many filenames specified\n", stderr);
   }

   // If there was an error then quit
   if (errcode)
      return EXIT_FAILURE;

   // Show tool version?
   if (show_ver) {
      puts("1.0");
      return EXIT_SUCCESS;
   }

   //
   FILE *out;
   if (outfilename == NULL)
      out = stdout;
   else {
      out = fopen(outfilename, "w");
      if (out == NULL) {
         fprintf(stderr, "Error: can't open output file \"%s\"\n",
            outfilename);
         return EXIT_FAILURE;
      }
   }

   // Show tool usage?
   if (show_help) {
      printf("Usage:\n"
             "  %s <options>\n"
             "\n"
             "Options:\n"
             "  -t or --title ....... Set game title\n"
             "  -c or --copyright ... Set copyright code\n"
             "\n"
             "  -6 or --6pad ........ Specify 6-pad support\n"
             "  -m ir --mouse ....... Specify mouse support\n"
             "  -cd or --megacd ..... Specify Mega CD support\n"
             "  -s or --sram ........ Specify SRAM support\n"
             "\n"
             "  -h or --help ........ Show this help\n"
             "  -v or --version ..... Show tool version\n"
             "\n"
             "The -t and -c options take an extra argument following them,\n"
             "for example: %s -t \"SONIC THE HEDGEHOG\" -c \"SEGA\".\n",
             argv[0], argv[0]);
      return EXIT_SUCCESS;
   }

   // Generate header
   errcode = generate_asm(out, &header);

   // If there was an error, show a message
   if (errcode) {
      // Determine message to show
      const char *msg;
      switch(errcode) {
         case ERR_CANTWRITE: msg = "can't write header"; break;
         default: msg = "unknown error"; break;
      }

      // Show message on screen
      fprintf(stderr, "Error: %s\n", msg);
   }

   // Quit program
   if (out != stdout) fclose(out);
   return errcode ? EXIT_FAILURE : EXIT_SUCCESS;
}

//***************************************************************************
// generate_asm
// Generates a Mega Drive header as assembly code.
//---------------------------------------------------------------------------
// param out: output file
// param header: header information
// return: error code
//***************************************************************************

int generate_asm(FILE *out, const HeaderInfo *header) {
   // Abbreviations of each month used in the copyright field
   static const char *months[] = {
      "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
      "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
   };

   // Fill device support string with the supported devices
   // The standard 3-pad is assumed to be always supported
   // Games not supporting it are not cool, OK?
   char devices[MAX_DEVICES+1];
   char *ptr = devices;
   *ptr++ = 'J';
   if (header->pad6) *ptr++ = '6';
   if (header->mouse) *ptr++ = 'M';
   if (header->megacd) *ptr++ = 'C';
   *ptr = '\0';

   // Header identification
   fputs("    dc.b    \"SEGA MEGA DRIVE \"\n", out);

   // Game information
   fprintf(out, "    dc.b    \"(C)%-4s %04u.%s\"\n", header->copyright,
      header->year, months[header->month]);
   fprintf(out, "    dc.b    \"%-48s\"\n", header->title);
   fprintf(out, "    dc.b    \"%-48s\"\n", header->title);
   fputs("    dc.b    \"GM \?\?\?\?\?\?\?\?-00\"\n", out);
   fputs("    dc.w    $0000\n", out);

   // Device support
   fprintf(out, "    dc.b    \"%-16s\"\n", devices);

   // ROM and RAM address ranges
   fputs("    dc.l    $000000, $3FFFFF\n", out);
   fputs("    dc.l    $FF0000, $FFFFFF\n", out);

   // SRAM support
   if (header->sram) {
      fputs("    dc.b    \"RA\", $F8, $20\n", out);
      fputs("    dc.l    $200001, $20FFFF\n", out);
   } else
      fputs("    dcb.b   12, $20\n", out);

   // Unused stuff
   fputs("    dcb.b   12, $20\n", out);
   fputs("    dcb.b   40, $20\n", out);

   // Region support
   fputs("    dc.b    \"JUE\"\n", out);

   // Unused stuff
   fputs("    dcb.b   13, $20\n", out);

   // Done
   return ferror(out) ? ERR_CANTWRITE : ERR_NONE;
}
