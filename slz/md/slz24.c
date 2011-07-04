// Required headers
#include <stdint.h>

//***************************************************************************
// decompress_slz24
// Decompresses data stored in SLZ24 format
//---------------------------------------------------------------------------
// param out: where to store decompressed data
// param in: pointer to SLZ24-compressed data
//***************************************************************************

void decompress_slz24(uint8_t *out, const uint8_t *in) {
   // Retrieve uncompressed size
   uint32_t size = in[0] << 16 | in[1] << 8 | in[2];
   in += 3;
   
   // To store the tokens
   uint8_t num_tokens = 1;
   uint8_t tokens;
   
   // Go through all compressed data until we're done decompressing
   while (size != 0) {
      // Need more tokens?
      num_tokens--;
      if (num_tokens == 0) {
         tokens = *in++;
         num_tokens = 8;
      }
      
      // Compressed string?
      if (tokens & 0x80) {
         // Get distance and length
         uint16_t dist = in[0] << 8 | in[1];
         uint8_t len = dist & 0x0F;
         dist = dist >> 4;
         in += 2;
         
         // Discount string length from size
         size -= len + 3;
         
         // Copy string using Duff's device
         // Code looks crazy, doesn't it? :)
         uint8_t *ptr = out - dist - 3;
         switch (len) {
            case 15: *out++ = *ptr++;
            case 14: *out++ = *ptr++;
            case 13: *out++ = *ptr++;
            case 12: *out++ = *ptr++;
            case 11: *out++ = *ptr++;
            case 10: *out++ = *ptr++;
            case  9: *out++ = *ptr++;
            case  8: *out++ = *ptr++;
            case  7: *out++ = *ptr++;
            case  6: *out++ = *ptr++;
            case  5: *out++ = *ptr++;
            case  4: *out++ = *ptr++;
            case  3: *out++ = *ptr++;
            case  2: *out++ = *ptr++;
            case  1: *out++ = *ptr++;
            case  0: *out++ = *ptr++;
                     *out++ = *ptr++;
                     *out++ = *ptr++;
         }
      }
      
      // Uncompressed byte?
      else {
         // Store byte as-is
         *out++ = *in++;
         size--;
      }
      
      // Go for next token
      tokens += tokens;
   }
}
