//***************************************************************************
// "main.c"
// Program entry point, parses command line and runs stuff as required
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
#include "main.h"
#include "batch.h"
#include "event.h"

//***************************************************************************
// Program entry point
//***************************************************************************

int main(int argc, char **argv) {
   // To know if there was an error or not
   int errcode = 0;

   // Scan all arguments
   int show_help = 0;
   int show_ver = 0;
   const char *filename = NULL;

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

      // Filename?
      else if (filename == NULL)
         filename = arg;

      // Too many files specified?
      else
         err_manyfiles = 1;
   }

   // Look for error conditions
   if (!show_help && !show_ver) {
      if (filename == NULL) {
         errcode = 1;
         fprintf(stderr, "Error: batch filename missing\n");
      } else if (err_manyfiles) {
         errcode = 1;
         fprintf(stderr, "Error: too many filenames specified\n");
      }
   }

   // If there was an error then quit
   if (errcode)
      return EXIT_FAILURE;

   // Show tool version?
   if (show_ver) {
      puts("0.8");
      return EXIT_SUCCESS;
   }

   // Show tool usage?
   if (show_help) {
      printf("Usage:\n"
             "  %s <batchfile>\n"
             "\n"
             "Options:\n"
             "  -h or --help ...... Show this help\n"
             "  -v or --version ... Show tool version\n",
             argv[0]);
      return EXIT_SUCCESS;
   }

   // Parse batch file
   errcode = process_batch(filename);

   // If there was an error, show a message
   if (errcode) {
      // Determine message to show
      const char *msg;
      switch(errcode) {
         case ERR_OPENBATCH: msg = "can't open batch file"; break;
         case ERR_READBATCH: msg = "can't read from batch file"; break;
         case ERR_NOMEMORY: msg = "ran out of memory"; break;
         case ERR_PARSE: msg = "unable to process batch file"; break;
         default: msg = "unknown error"; break;
      }

      // Show message on screen
      fprintf(stderr, "Error: %s\n", msg);
   }

   // Quit program
   reset_events();
   return errcode ? EXIT_FAILURE : EXIT_SUCCESS;
}
