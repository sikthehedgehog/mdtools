#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "instruments.h"
#include "esf.h"
#include "vgm.h"
#include "gd3.h"

// Program version
#define VERSION "1.0"

//***************************************************************************
// main
// Program entry point.
//---------------------------------------------------------------------------
// param argc ... number of command line arguments
// param argv ... list of command line arguments
//---------------------------------------------------------------------------
// return ....... EXIT_SUCCESS if all went OK, EXIT_FAILURE otherwise
//***************************************************************************

int main(int argc, char **argv)
{
   // If any of the arguments is --version then show the version
   for (int i = 1; i < argc; i++) {
      const char *arg = argv[i];
      if (!strcmp(arg, "--version") || !strcmp(arg, "-v")) {
         puts(VERSION);
         return EXIT_SUCCESS;
      }
   }
   
   // Make sure we have enough arguments
   if (argc < 4 || argc > 9) {
      fprintf(stderr, "Usage: %s <instruments.txt> "
                      "<track.esf> <track.vgm> "
                      "[track-title] [game-title] [composer] "
                      "[release] [ripped-by]\n",
                      argv[0]);
      return EXIT_FAILURE;
   }
   const char *listname = argv[1];
   const char *esfname = argv[2];
   const char *vgmname = argv[3];
   const char *tracktitle = (argc >= 5) ? argv[4] : "";
   const char *gametitle = (argc >= 6) ? argv[5] : "";
   const char *composer = (argc >= 7) ? argv[6] : "";
   const char *release = (argc >= 8) ? argv[7] : "";
   const char *rippedby = (argc >= 9) ? argv[8] : "";
   
   // Load all instruments
   if (load_instruments(listname)) {
      return EXIT_FAILURE;
   }
   
   // Parse ESF file
   if (parse_esf(esfname)) {
      return EXIT_FAILURE;
   }
   
   // Generate GD3 info
   set_track_title(tracktitle);
   set_game_title(gametitle);
   set_composer(composer);
   set_release(release);
   set_rippedby(rippedby);
   compile_gd3();
   
   // Generate VGM
   if (save_vgm(vgmname)) {
      return EXIT_FAILURE;
   }
   
   // We're done
   return EXIT_SUCCESS;
}
