#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "instruments.h"
#include "util.h"

// Maximum possible number of instruments
// This is an Echo limit!
#define MAX_INSTRUMENTS 0x100

// Instruments loaded into RAM
static Blob *instruments[MAX_INSTRUMENTS] = { NULL };
static uint8_t used_as_pcm[MAX_INSTRUMENTS] = { 0 };
static unsigned num_instruments = 0;

// PCM block list
static Blob *pcm_blob = NULL;
static unsigned pcm_map[MAX_INSTRUMENTS];
static unsigned next_pcm_id = 0;

// Dummy blob for when an instrument is missing
static const Blob dummy_blob = { 0 };

//***************************************************************************
// load_instruments
// Loads all instruments into RAM.
//---------------------------------------------------------------------------
// param listname ... instrument list filename
//---------------------------------------------------------------------------
// return ........... 0 on success, -1 on failure
//***************************************************************************

int load_instruments(const char *listname)
{
   // Open instrument list file
   FILE *listfile = fopen(listname, "r");
   if (listfile == NULL) {
      fprintf(stderr, "Error: can't open instrument list \"%s\"\n",
                      listname);
      return -1;
   }
   
   // Scan the whole instrument list
   for (;;) {
      // Read next instrument filename
      // Make sure to skip blank lines
      char *line = read_line(listfile);
      if (line == NULL) break;
      if (!line[0]) { free(line); continue; }
      
      // Load instrument into memory
      Blob *blob = load_file(line);
      if (blob == NULL) {
         fprintf(stderr, "Warning: can't load instrument \"%s\"\n", line);
      }
      instruments[num_instruments] = blob;
      
      // Onto next instrument
      num_instruments++;
      free(line);
   }
   
   // We're done
   fclose(listfile);
   return 0;
}

//***************************************************************************
// get_instrument
// Retrieves the data for an instrument.
//---------------------------------------------------------------------------
// param id ... instrument ID (0..255)
//---------------------------------------------------------------------------
// return ..... blob with instrument data
//***************************************************************************

const Blob *get_instrument(unsigned id) {
   // If the instrument was loaded, return it
   // If the instrument is missing, return the dummy blob
   return (instruments[id] != NULL) ? instruments[id] : &dummy_blob;
}

//***************************************************************************
// mark_as_pcm
// Marks an instrument as having been used for PCM.
//---------------------------------------------------------------------------
// param id ... instrument ID (0..255)
//***************************************************************************

void mark_as_pcm(unsigned id)
{
   // Check if already allocated
   if (used_as_pcm[id]) return;
   used_as_pcm[id] = 1;
   
   // Generate PCM block header
   // Note that we substract 1 from the PCM size
   // to remove the 0xFF terminator
   const uint8_t *data = instruments[id]->data;
   size_t size = instruments[id]->size - 1;
   
   uint8_t header[] = {
      0x67, 0x66, 0x00,
      (uint8_t)(size),
      (uint8_t)(size >> 8),
      (uint8_t)(size >> 16),
      (uint8_t)(size >> 24),
   };
   
   // Add it to the PCM block
   size_t total_size;
   if (pcm_blob == NULL) total_size = 0;
   else total_size = pcm_blob->size;
   
   size_t offset = total_size;
   total_size += 7 + size;
   
   pcm_blob = realloc(pcm_blob, sizeof(Blob) + total_size);
   if (pcm_blob == NULL) abort();
   pcm_blob->size = total_size;
   
   memcpy(&pcm_blob->data[offset+0], header, 7);
   memcpy(&pcm_blob->data[offset+7], data, size);
   
   // Store details
   pcm_map[id] = next_pcm_id;
   next_pcm_id++;
}

//***************************************************************************
// get_pcm_id
// Gets the PCM block ID for an instrument ID.
//---------------------------------------------------------------------------
// param id ... instrument ID
//---------------------------------------------------------------------------
// return ..... PCM block ID
//***************************************************************************

unsigned get_pcm_id(unsigned id)
{
   mark_as_pcm(id);
   return pcm_map[id];
}

//***************************************************************************
//
//***************************************************************************

const Blob *get_pcm_blob(void)
{
   return pcm_blob;
}
