// Required headers
#include <stdint.h>

//***************************************************************************
// decompress_uftc
// Decompresses tiles stored in UFTC format
//---------------------------------------------------------------------------
// param out: where to store decompressed tiles
// param in: pointer to UFTC-compressed data
// param start: ID of first tile to decompress (counting from 0)
// param count: how many tiles to decompress
//***************************************************************************

void decompress_uftc(uint16_t *out, const uint16_t *in, uint16_t start,
uint16_t count) {
   __asm__ __volatile__ (
      "moveq #0,%%d2\n\t"           // Get size of dictionary
      "move.w (%0)+,%%d2\n\t"
      
      "lea (%0,%%d2.l),%%a4\n\t"    // Get address of data with first tile
      "and.l #65535,%2\n\t"            // to be decompressed (using a dword
      "lsl.l #3,%2\n\t"                // so we can have up to 8192 tiles)
      "lea (%%a4,%2.l),%%a4\n\t"
      
      "bra 2f\n"                    // Start decompressing
      
   "1: move.w (%%a4)+,%%d2\n\t"     // Fetch addresses of dictionary
      "lea (%0,%%d2.l),%%a2\n\t"       // entries for the first two 4x4
      "move.w (%%a4)+,%%d2\n\t"        // blocks of this tile
      "lea (%0,%%d2.l),%%a3\n\t"
      
      "move.w (%%a2)+,(%1)+\n\t"    // Decompress first pair of 4x4 blocks
      "move.w (%%a3)+,(%1)+\n\t"       //  into the output buffer
      "move.w (%%a2)+,(%1)+\n\t"
      "move.w (%%a3)+,(%1)+\n\t"
      
      "move.w (%%a4)+,%%d2\n\t"     // Fetch addresses of dictionary
      "lea (%0,%%d2.l),%%a2\n\t"       // entries for the last two 4x4
      "move.w (%%a4)+,%%d2\n\t"        // blocks of this tile
      "lea (%0,%%d2.l),%%a3\n\t"
      
      "move.w (%%a2)+,(%1)+\n\t"    // Decompress last pair of 4x4 blocks
      "move.w (%%a3)+,(%1)+\n\t"       //  into the output buffer
      "move.w (%%a2)+,(%1)+\n\t"
      "move.w (%%a3)+,(%1)+\n"
      
   "2: dbf %3,1b"                   // Go for next tile
      
      : "=a"(in),"=a"(out),"=d"(start),"=d"(count)
      : "0"(in), "1"(out), "2"(start), "3"(count)
      : "d2","a2","a3","a4","memory","cc");
}
