// Required headers
#include <stdint.h>

//***************************************************************************
// decompress_uftc15
// Decompresses tiles stored in UFTC15 format
//---------------------------------------------------------------------------
// This is the original UFTC15 code. This is kept here in case somebody needs
// it, but ideally you should switch to UFTC16 instead.
//---------------------------------------------------------------------------
// param out: where to store decompressed tiles
// param in: pointer to UFTC-compressed data
// param start: ID of first tile to decompress (counting from 0)
// param count: how many tiles to decompress
//***************************************************************************

void decompress_uftc15(int16_t *out, const int16_t *in, int16_t start,
int16_t count) {
   // Get size of dictionary
   int16_t dirsize = *in++;
   
   // Get addresses of dictionary and first tile to decompress
   int16_t *dir = in;
   in += dirsize + (start << 4);
   
   // Decompress all tiles
   for (; count != 0; count--) {
      // To store pointers to 4x4 blocks
      int16_t *block1, *block2;
      
      // Retrieve location in the dictionary of first pair of 4x4 blocks
      block1 = dir + *in++;
      block2 = dir + *in++;
      
      // Decompress first pair of 4x4 blocks
      *out++ = *block1++;
      *out++ = *block2++;
      *out++ = *block1++;
      *out++ = *block2++;
      *out++ = *block1++;
      *out++ = *block2++;
      *out++ = *block1++;
      *out++ = *block2++;
      
      // Retrieve location in the dictionary of second pair of 4x4 blocks
      block1 = dir + *in++;
      block2 = dir + *in++;
      
      // Decompress second pair of 4x4 blocks
      *out++ = *block1++;
      *out++ = *block2++;
      *out++ = *block1++;
      *out++ = *block2++;
      *out++ = *block1++;
      *out++ = *block2++;
      *out++ = *block1++;
      *out++ = *block2++;
   }
}
