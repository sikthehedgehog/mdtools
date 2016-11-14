// Required headers
#include <stdio.h>
#include <stdlib.h>
#include "mml.h"
#include "esf.h"

//***************************************************************************
// main
// Program entry point.
//***************************************************************************

int main(int argc, char **argv)
{
   // Check arguments
   if (argc != 3) {
      fprintf(stderr, "Usage: %s <infile.mml> <outfile.esf>\n", argv[0]);
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
