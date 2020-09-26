#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "instruments.h"
#include "stream.h"
#include "vgm.h"
#include "gd3.h"
#include "util.h"

// Some defines for the file format
#define HEADER_VERSION           0x08        // VGM format version
#define HEADER_VGMOFFSET         0x34        // VGM data offset
#define HEADER_TOTALSAMPLES      0x18        // VGM length in samples
#define HEADER_LOOPOFFSET        0x1C        // Loop offset in bytes
#define HEADER_LOOPSAMPLES       0x20        // Loop length in samples
#define HEADER_GD3OFFSET         0x14        // GD3 data offset
#define HEADER_EOFOFFSET         0x04        // Offset to end of file
#define HEADER_YMCLOCK           0x2C        // YM2612 clock frequency
#define HEADER_PSGCLOCK          0x0C        // PSG clock frequency
#define HEADER_PSGNOISEFEEDBACK  0x28        // PSG noise feedback
#define HEADER_PSGNOISEWIDTH     0x2A        // PSG noise width

#define HEADERSIZE            0x100          // VGM header size
#define VERSION               0x160          // VGM format version
#define YMCLOCK               7670454        // YM2612 clock frequency
#define PSGCLOCK              3579545        // PSG clock frequency
#define PSGNOISEFEEDBACK      9              // 9 for SMS/MD
#define PSGNOISEWIDTH         16             // 16 for SMS/MD

// VGM commands we use
#define VGMCMD_DELAY          0x61        // Delay
#define VGMCMD_YMREG0         0x52        // Write YM2612 register (bank 0)
#define VGMCMD_YMREG1         0x53        // Write YM2612 register (bank 1)
#define VGMCMD_PSGREG         0x50        // Write PSG register
#define VGMCMD_SETUPPCMCHIP   0x90        // Set up PCM stream chip
#define VGMCMD_SETUPPCMDATA   0x91        // Set up PCM stream data
#define VGMCMD_STARTPCM       0x95        // Start PCM stream
#define VGMCMD_STOPPCM        0x94        // Stop PCM stream
#define VGMCMD_SETPCMFREQ     0x92        // Set PCM stream sample rate
#define VGMCMD_END            0x66        // End of stream

//***************************************************************************
// save_vgm
// Generates and saves the resulting VGM file.
//---------------------------------------------------------------------------
// param vgmname ... VGM filename
//---------------------------------------------------------------------------
// return .......... 0 on success, -1 on failure
//***************************************************************************

int save_vgm(const char *vgmname)
{
   // Try to create new file
   FILE *vgmfile = fopen(vgmname, "wb");
   if (vgmfile == NULL) {
      fprintf(stderr, "Error: can't create VGM file \"%s\"\n", vgmname);
      return -1;
   }
   
   // Retrieve PCM block data
   size_t pcm_size = 0;
   const Blob *pcm_blob = get_pcm_blob();
   if (pcm_blob != NULL) pcm_size = pcm_blob->size;
   
   // Build VGM header
   uint8_t header[HEADERSIZE] = { 'V','g','m',' ' };
   const Blob *gd3 = get_gd3_blob();
   
   unsigned len_in_bytes = get_num_stream_bytes();
   unsigned len_in_samples = get_num_stream_samples();
   unsigned vgm_offset = HEADERSIZE - HEADER_VGMOFFSET;
   unsigned gd3_offset = HEADERSIZE + len_in_bytes + pcm_size -
                                      HEADER_GD3OFFSET;
   unsigned eof_offset = HEADERSIZE + len_in_bytes + pcm_size +
                                      gd3->size - HEADER_EOFOFFSET;
   
   unsigned has_loop = does_stream_loop();
   unsigned loop_offset = has_loop ?
                          (get_loop_offset() + pcm_size
                           + HEADERSIZE - HEADER_LOOPOFFSET) : 0;
   unsigned loop_length = has_loop ?
                          get_num_loop_samples() : 0;
   
   header[HEADER_VERSION+0] = (uint8_t)(VERSION);
   header[HEADER_VERSION+1] = (uint8_t)(VERSION >> 8);
   header[HEADER_VERSION+2] = (uint8_t)(VERSION >> 16);
   header[HEADER_VERSION+3] = (uint8_t)(VERSION >> 24);
   
   header[HEADER_TOTALSAMPLES+0] = (uint8_t)(len_in_samples);
   header[HEADER_TOTALSAMPLES+1] = (uint8_t)(len_in_samples >> 8);
   header[HEADER_TOTALSAMPLES+2] = (uint8_t)(len_in_samples >> 16);
   header[HEADER_TOTALSAMPLES+3] = (uint8_t)(len_in_samples >> 24);
   header[HEADER_VGMOFFSET+0] = (uint8_t)(vgm_offset);
   header[HEADER_VGMOFFSET+1] = (uint8_t)(vgm_offset >> 8);
   header[HEADER_VGMOFFSET+2] = (uint8_t)(vgm_offset >> 16);
   header[HEADER_VGMOFFSET+3] = (uint8_t)(vgm_offset >> 24);
   header[HEADER_LOOPOFFSET+0] = (uint8_t)(loop_offset);
   header[HEADER_LOOPOFFSET+1] = (uint8_t)(loop_offset >> 8);
   header[HEADER_LOOPOFFSET+2] = (uint8_t)(loop_offset >> 16);
   header[HEADER_LOOPOFFSET+3] = (uint8_t)(loop_offset >> 24);
   header[HEADER_LOOPSAMPLES+0] = (uint8_t)(loop_length);
   header[HEADER_LOOPSAMPLES+1] = (uint8_t)(loop_length >> 8);
   header[HEADER_LOOPSAMPLES+2] = (uint8_t)(loop_length >> 16);
   header[HEADER_LOOPSAMPLES+3] = (uint8_t)(loop_length >> 24);
   header[HEADER_GD3OFFSET+0] = (uint8_t)(gd3_offset);
   header[HEADER_GD3OFFSET+1] = (uint8_t)(gd3_offset >> 8);
   header[HEADER_GD3OFFSET+2] = (uint8_t)(gd3_offset >> 16);
   header[HEADER_GD3OFFSET+3] = (uint8_t)(gd3_offset >> 24);
   header[HEADER_EOFOFFSET+0] = (uint8_t)(eof_offset);
   header[HEADER_EOFOFFSET+1] = (uint8_t)(eof_offset >> 8);
   header[HEADER_EOFOFFSET+2] = (uint8_t)(eof_offset >> 16);
   header[HEADER_EOFOFFSET+3] = (uint8_t)(eof_offset >> 24);
   
   header[HEADER_YMCLOCK+0] = (uint8_t)(YMCLOCK);
   header[HEADER_YMCLOCK+1] = (uint8_t)(YMCLOCK >> 8);
   header[HEADER_YMCLOCK+2] = (uint8_t)(YMCLOCK >> 16);
   header[HEADER_YMCLOCK+3] = (uint8_t)(YMCLOCK >> 24);
   header[HEADER_PSGCLOCK+0] = (uint8_t)(PSGCLOCK);
   header[HEADER_PSGCLOCK+1] = (uint8_t)(PSGCLOCK >> 8);
   header[HEADER_PSGCLOCK+2] = (uint8_t)(PSGCLOCK >> 16);
   header[HEADER_PSGCLOCK+3] = (uint8_t)(PSGCLOCK >> 24);
   header[HEADER_PSGNOISEFEEDBACK+0] = (uint8_t)(PSGNOISEFEEDBACK);
   header[HEADER_PSGNOISEFEEDBACK+1] = (uint8_t)(PSGNOISEFEEDBACK >> 8);
   header[HEADER_PSGNOISEWIDTH] = (uint8_t)(PSGNOISEWIDTH);
   
   // Write VGM header
   if (fwrite(header, 1, HEADERSIZE, vgmfile) < HEADERSIZE) {
      goto error;
   }
   
   // Write PCM block
   if (pcm_size != 0)
   if (fwrite(pcm_blob->data, 1, pcm_size, vgmfile) < pcm_size) {
      goto error;
   }
   
   // Go through entire stream
   uint8_t buffer[0x10];
   unsigned num_commands = get_num_stream_commands();
   for (unsigned id = 0; id < num_commands; id++) {
      // Get next command
      const StreamCmd *cmd = get_stream_command(id);
      switch (cmd->type) {
         // Delay
         case STREAM_DELAY: {
            buffer[0] = VGMCMD_DELAY;
            buffer[1] = (uint8_t)(cmd->value1);
            buffer[2] = (uint8_t)(cmd->value1 >> 8);
            if (fwrite(buffer, 1, 3, vgmfile) < 3) goto error;
         } break;
         
         // YM2612 register writes
         case STREAM_YMREG0:
         case STREAM_YMREG1: {
            buffer[0] = cmd->type - STREAM_YMREG0 + VGMCMD_YMREG0;
            //buffer[0] = VGMCMD_YMREG0;
            buffer[1] = cmd->value1;
            buffer[2] = cmd->value2;
            if (fwrite(buffer, 1, 3, vgmfile) < 3) goto error;
         } break;
         
         // PSG register writes
         case STREAM_PSGREG: {
            buffer[0] = VGMCMD_PSGREG;
            buffer[1] = cmd->value1;
            if (fwrite(buffer, 1, 2, vgmfile) < 2) goto error;
         } break;
         
         // Setup PCM streams
         case STREAM_INITPCM: {
            buffer[0] = VGMCMD_SETUPPCMCHIP;
            buffer[1] = 0x00;
            buffer[2] = 0x02;
            buffer[3] = 0x00;
            buffer[4] = 0x2A;
            buffer[5] = VGMCMD_SETUPPCMDATA;
            buffer[6] = 0x00;
            buffer[7] = 0x00;
            buffer[8] = 0x01;
            buffer[9] = 0x00;
            if (fwrite(buffer, 1, 10, vgmfile) < 10) goto error;
         } break;
         
         // Start PCM stream
         case STREAM_STARTPCM: {
            buffer[0] = VGMCMD_STARTPCM;
            buffer[1] = 0x00;
            buffer[2] = (uint8_t)(cmd->value1);
            buffer[3] = (uint8_t)(cmd->value1 >> 8);
            buffer[4] = 0x00;
            if (fwrite(buffer, 1, 5, vgmfile) < 5) goto error;
         } break;
         
         // Stop PCM stream
         case STREAM_STOPPCM: {
            buffer[0] = VGMCMD_STOPPCM;
            buffer[1] = 0x00;
            if (fwrite(buffer, 1, 2, vgmfile) < 2) goto error;
         } break;
         
         // Set PCM streaming frequency
         case STREAM_SETPCMFREQ: {
            buffer[0] = VGMCMD_SETPCMFREQ;
            buffer[1] = 0x00;
            buffer[2] = (uint8_t)(cmd->value1);
            buffer[3] = (uint8_t)(cmd->value1 >> 8);
            buffer[4] = (uint8_t)(cmd->value1 >> 16);
            buffer[5] = (uint8_t)(cmd->value1 >> 24);
            if (fwrite(buffer, 1, 6, vgmfile) < 6) goto error;
         } break;
         
         // End of stream
         case STREAM_END: {
            buffer[0] = VGMCMD_END;
            if (fwrite(buffer, 1, 1, vgmfile) < 1) goto error;
         } break;
         
         // uh oh
         default: {
            fprintf(stderr,
                    "[INTERNAL] Warning: unhandled command type %u!\n",
                    cmd->type);
         } break;
      }
   }
   
   // Write GD3 blob
   if (fwrite(gd3->data, 1, gd3->size, vgmfile) < gd3->size)
      goto error;
   
   // We're done
   fclose(vgmfile);
   return 0;
   
error:
   fprintf(stderr, "Error: can't write to VGM file \"%s\"\n", vgmname);
   fclose(vgmfile);
   return -1;
}
