//***************************************************************************
// "main.c"
// Program entry point, parses command line and runs stuff as required
//***************************************************************************
// TFI to EIF conversion tool
// Copyright 2011 Javier Degirolmo
//
// This file is part of tfi2eif.
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//***************************************************************************

// Required headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "tfi.h"
#include "eif.h"

//***************************************************************************
// Program entry point
//***************************************************************************

int main(int argc, char **argv) {
   // To know if there was an error or not
   int errcode = 0;

   // Scan all arguments
   int show_help = 0;
   int show_ver = 0;
   const char *infilename = NULL;
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

      // Input filename?
      else if (infilename == NULL)
         infilename = arg;

      // Output filename?
      else if (outfilename == NULL)
         outfilename = arg;

      // Too many files specified?
      else
         err_manyfiles = 1;
   }

   // Look for error conditions
   if (!show_help && !show_ver) {
      if (infilename == NULL) {
         errcode = 1;
         fprintf(stderr, "Error: input filename missing\n");
      } else if (outfilename == NULL) {
         errcode = 1;
         fprintf(stderr, "Error: output filename missing\n");
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
      puts("1.0b");
      return EXIT_SUCCESS;
   }

   // Show tool usage?
   if (show_help) {
      printf("Usage:\n"
             "  %s <infile> <outfile>\n"
             "\n"
             "Options:\n"
             "  -h or --help ...... Show this help\n"
             "  -v or --version ... Show tool version\n",
             argv[0]);
      return EXIT_SUCCESS;
   }

   // Open input file
   FILE *infile = fopen(infilename, "rb");
   if (infile == NULL) {
      fprintf(stderr, "Error: can't open input file \"%s\"\n", infilename);
      return EXIT_FAILURE;
   }

   // Open output file
   FILE *outfile = fopen(outfilename, "wb");
   if (outfile == NULL) {
      fprintf(stderr, "Error: can't open output file \"%s\"\n", outfilename);
      fclose(infile);
      return EXIT_FAILURE;
   }

   // Try to read FM instrument from TFI file
   Instrument instr;
   errcode = read_tfi(infile, &instr);

   // Try to write FM instrument into EIF file
   if (!errcode)
      errcode = write_eif(outfile, &instr);

   // If there was an error, show a message
   if (errcode) {
      // Determine message to show
      const char *msg;
      switch(errcode) {
         case ERR_CANTREAD: msg = "can't read from input file"; break;
         case ERR_CANTWRITE: msg = "can't write to output file"; break;
         case ERR_CORRUPT: msg = "input file isn't a valid TFM Maker "
            "instrument"; break;
         default: msg = "unknown error"; break;
      }

      // Show message on screen
      fprintf(stderr, "Error: %s\n", msg);
   }

   // Quit program
   fclose(outfile);
   fclose(infile);
   return errcode ? EXIT_FAILURE : EXIT_SUCCESS;
}
