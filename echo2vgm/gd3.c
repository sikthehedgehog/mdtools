#include <stdlib.h>
#include <string.h>
#include "gd3.h"
#include "util.h"

// Track information
static const char *title = "";         // Track title
static const char *game = "";          // Game title
static const char *composer = "";      // Composer name
static const char *release = "";       // Release date
static const char *rippedby = "";      // Ripper name

// Blob where we hold the compiled GD3
Blob *gd3 = NULL;

//***************************************************************************
// set_track_title
// Changes the track title.
//---------------------------------------------------------------------------
// param text ... new title (UTF-8 encoded)
//---------------------------------------------------------------------------
// NOTE: string must stay around until the GD3 data is built!
//***************************************************************************

void set_track_title(const char *text)
{
   title = text;
}

//***************************************************************************
// set_game_title
// Changes the game title.
//---------------------------------------------------------------------------
// param text ... new title (UTF-8 encoded)
//---------------------------------------------------------------------------
// NOTE: string must stay around until the GD3 data is built!
//***************************************************************************

void set_game_title(const char *text)
{
   game = text;
}

//***************************************************************************
// set_composer
// Changes the composer name.
//---------------------------------------------------------------------------
// param text ... new composer (UTF-8 encoded)
//---------------------------------------------------------------------------
// NOTE: string must stay around until the GD3 data is built!
//***************************************************************************

void set_composer(const char *text)
{
   composer = text;
}

//***************************************************************************
// set_release
// Changes the release date.
//---------------------------------------------------------------------------
// param text ... new release date (UTF-8 encoded)
//---------------------------------------------------------------------------
// NOTE: string must stay around until the GD3 data is built!
//***************************************************************************

void set_release(const char *text)
{
   release = text;
}

//***************************************************************************
// set_rippedby
// Changes the ripper name.
//---------------------------------------------------------------------------
// param text ... new ripper (UTF-8 encoded)
//---------------------------------------------------------------------------
// NOTE: string must stay around until the GD3 data is built!
//***************************************************************************

void set_rippedby(const char *text)
{
   rippedby = text;
}

//***************************************************************************
// compile_gd3
// Builds the GD3 blob with the provided track info.
//***************************************************************************

void compile_gd3(void)
{
   // Build all strings
   uint16_t *u_title = utf8_to_utf16(title);
   uint16_t *u_game = utf8_to_utf16(game);
   uint16_t *u_system = utf8_to_utf16("Sega Mega Drive / Genesis");
   uint16_t *u_composer = utf8_to_utf16(composer);
   uint16_t *u_release = utf8_to_utf16(release);
   uint16_t *u_rippedby = utf8_to_utf16(rippedby);
   uint16_t *u_notes = utf8_to_utf16("");
   
   // Compute space usage
   size_t size_title = get_utf16_size(u_title);
   size_t size_game = get_utf16_size(u_game);
   size_t size_system = get_utf16_size(u_system);
   size_t size_composer = get_utf16_size(u_composer);
   size_t size_release = get_utf16_size(u_release);
   size_t size_rippedby = get_utf16_size(u_rippedby);
   size_t size_notes = get_utf16_size(u_notes);
   
   size_t text_size = 2*size_title +
                      2*size_game +
                      2*size_system +
                      2*size_composer +
                      size_release +
                      size_rippedby +
                      size_notes;
   
   size_t gd3_size = 12 + text_size;
   gd3 = alloc_blob(gd3_size);
   
   // Write the header
   uint8_t *ptr = gd3->data;
   *ptr++ = 'G';  *ptr++ = 'd';  *ptr++ = '3';  *ptr++ = ' ';
   *ptr++ = 0x00; *ptr++ = 0x01; *ptr++ = 0x00; *ptr++ = 0x00;
   *ptr++ = (uint8_t)(gd3_size);
   *ptr++ = (uint8_t)(gd3_size >> 8);
   *ptr++ = (uint8_t)(gd3_size >> 16);
   *ptr++ = (uint8_t)(gd3_size >> 24);
   
   // Write the text
   memcpy(ptr, u_title, size_title); ptr += size_title;
   memcpy(ptr, u_title, size_title); ptr += size_title;
   memcpy(ptr, u_game, size_game); ptr += size_game;
   memcpy(ptr, u_game, size_game); ptr += size_game;
   memcpy(ptr, u_system, size_system); ptr += size_system;
   memcpy(ptr, u_system, size_system); ptr += size_system;
   memcpy(ptr, u_composer, size_composer); ptr += size_composer;
   memcpy(ptr, u_composer, size_composer); ptr += size_composer;
   memcpy(ptr, u_release, size_release); ptr += size_release;
   memcpy(ptr, u_rippedby, size_rippedby); ptr += size_rippedby;
   memcpy(ptr, u_notes, size_notes); ptr += size_notes;
   
   // We're done
   free(u_title);
   free(u_game);
   free(u_system);
   free(u_composer);
   free(u_release);
   free(u_rippedby);
   free(u_notes);
}

//***************************************************************************
// get_gd3_blob
// Returns a pointer to the compiled GD3 blob.
//---------------------------------------------------------------------------
// return ... pointer to GD3 blob (NULL if not built yet)
//***************************************************************************

const Blob *get_gd3_blob(void)
{
   return gd3;
}
