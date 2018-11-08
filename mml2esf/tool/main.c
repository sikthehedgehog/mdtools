// Required headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mml.h"
#include "esf.h"

//***************************************************************************
// show_version
// Shows the tool version.
//***************************************************************************

void show_version(void)
{
   puts("1.2a");
}

//***************************************************************************
// show_usage
// Shows the tool help.
//---------------------------------------------------------------------------
// param name: program name (as invoked)
//***************************************************************************

void show_usage(const char *name)
{
   fprintf(stderr, "Usage: %s <infile.mml> <outfile.esf>\n", name);
}

//***************************************************************************
// main
// Program entry point.
//***************************************************************************

int main(int argc, char **argv)
{
   // Check arguments
   if (argc == 2) {
      if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version")) {
         show_version();
         return EXIT_SUCCESS;
      }
      if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") ||
      strcmp(argv[1], "-?") == 0) {
         show_usage(argv[0]);
         return EXIT_SUCCESS;
      }
   }
   if (argc != 3) {
      show_usage(argv[0]);
      return EXIT_FAILURE;
   }

   // Parse in the PMD file
   if (parse_mml(argv[1]))
      return EXIT_FAILURE;

   // Generate the ESF file
   if (generate_esf(argv[2]))
      return EXIT_FAILURE;

   // We're done
   return EXIT_SUCCESS;
}
