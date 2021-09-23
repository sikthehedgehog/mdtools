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
   // Get size of dictionary
   uint16_t dirsize = *in++;
   
   // Get addresses of dictionary and first tile to decompress
   uint16_t *dir = in;
   in += dirsize + (start << 4);
   
   // Decompress all tiles
   for (; count != 0; count--) {
      // To store pointers to 4x4 blocks
      uint16_t *block1, *block2;
      
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
