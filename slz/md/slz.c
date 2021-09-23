// Required headers
#include <stdint.h>

//***************************************************************************
// decompress_slz
// Decompresses data stored in SLZ format
//---------------------------------------------------------------------------
// param out: where to store decompressed data
// param in: pointer to SLZ-compressed data
//***************************************************************************

void decompress_slz(uint8_t *out, const uint8_t *in) {
   __asm__ __volatile__ (
      "move.b (%0)+,%%d0\n\t"       // Read uncompressed size
      "lsl.w #8,%%d0\n\t"
      "move.b (%0)+,%%d0\n\t"
      
      "moveq #1,%%d1\n"             // Cause code to fetch new token data
                                    // as soon as it starts
   "1: tst.w %%d0\n\t"              // Did we read all data?
      "beq 5f\n\t"                     // If so, we're done with it!
      
      "subq.w #1,%%d1\n\t"          // Check if we need more tokens
      "bne.s 2f\n\t"
      "move.b (%0)+,%%d2\n\t"
      "moveq #8,%%d1\n"
      
   "2: add.b %%d2,%%d2\n\t"         // Get next token type
      "bcc.s 4f\n\t"                   // 0 = uncompressed, 1 = compressed
      
      "move.b (%0)+,%%d3\n\t"       // Compressed? Read string info
      "lsl.w #8,%%d3\n\t"              // %d3 = distance
      "move.b (%0)+,%%d3\n\t"          // %d4 = length
      "move.b %%d3,%%d4\n\t"
      "lsr.w #4,%%d3\n\t"
      "and.w #15,%%d4\n\t"
      
      "subq.w #3,%%d0\n\t"          // Length is offset by 3
      "sub.w %%d4,%%d0\n\t"         // Now that we know the string length,
                                       // discount it from the amount of data
                                       // to be read
      
      "addq.w #3,%%d3\n\t"          // Distance is offset by 3
      "neg.w %%d3\n\t"              // Make distance go backwards
      
      "add.w %%d4,%%d4\n\t"         // Copy bytes using Duff's device
      "add.w %%d4,%%d4\n\t"            // MUCH faster than a loop, due to lack
      "eor.w #60,%%d4\n\t"             // of iteration overhead
      "jmp 3f(%%pc,%%d4.w)\n"
   "3: move.b (%1,%%d3.w),(%1)+\n\t"
      "move.b (%1,%%d3.w),(%1)+\n\t"
      "move.b (%1,%%d3.w),(%1)+\n\t"
      "move.b (%1,%%d3.w),(%1)+\n\t"
      "move.b (%1,%%d3.w),(%1)+\n\t"
      "move.b (%1,%%d3.w),(%1)+\n\t"
      "move.b (%1,%%d3.w),(%1)+\n\t"
      "move.b (%1,%%d3.w),(%1)+\n\t"
      "move.b (%1,%%d3.w),(%1)+\n\t"
      "move.b (%1,%%d3.w),(%1)+\n\t"
      "move.b (%1,%%d3.w),(%1)+\n\t"
      "move.b (%1,%%d3.w),(%1)+\n\t"
      "move.b (%1,%%d3.w),(%1)+\n\t"
      "move.b (%1,%%d3.w),(%1)+\n\t"
      "move.b (%1,%%d3.w),(%1)+\n\t"
      "move.b (%1,%%d3.w),(%1)+\n\t"
      "move.b (%1,%%d3.w),(%1)+\n\t"
      "move.b (%1,%%d3.w),(%1)+\n\t"
      
      "bra 1b\n"                    // Keep processing data
      
   "4: move.b (%0)+,(%1)+\n\t"      // Uncompressed? Read as is
      "subq.w #1,%%d0\n\t"          // It's always one byte long
      "bra 1b\n"                    // Keep processing data
      
   "5:"
      : "=a"(in),"=a"(out) : "0"(in),"1"(out) :
      "d0","d1","d2","d3","d4","memory","cc");
}
