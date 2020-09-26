#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "util.h"

// Largest file that load_file is willing to load
// 4MB = maximum size that Echo may be able to see
#define MAX_BLOBSIZE 0x400000

//***************************************************************************
// read_line
//
// Reads a line from a text file. Returns a pointer to the string, you should
// call free() when you're done with it. Returns NULL if it can't read any
// more lines from the file (usually means EOF).
//---------------------------------------------------------------------------
// param file ... file handle
//---------------------------------------------------------------------------
// return ....... pointer to string (NULL if can't read from file)
//***************************************************************************

char *read_line(FILE *file)
{
   char *buffer;
   size_t buflen = 0;
   ssize_t len;
   
   // Try to read a line from the file
   // We tell getline() to allocate a buffer for us
   len = getline(&buffer, &buflen, file);
   
   // Success, return the buffer with the line
   // Note that we may need to trim off the newline
   if (len != -1) {
      if (buffer[len-1] == '\n') {
         buffer[len-1] = '\0';
      }
      return buffer;
   }
   
   // Couldn't read line, return NULL
   // Yes, we have to free the buffer anyway
   else {
      free(buffer);
      return NULL;
   }
}

//***************************************************************************
// load_file
// Loads an entire file into RAM. On success, it returns a pointer to the
// blob (call free() when you're done with it). On failure it returns NULL.
//---------------------------------------------------------------------------
// param filename ... filename of file to load
//---------------------------------------------------------------------------
// return ........... pointer to blob on success, NULL on failure
//---------------------------------------------------------------------------
// notes: it'll refuse to load files that are too large
//***************************************************************************

Blob *load_file(const char *filename)
{
   // Try to open file
   FILE *file = fopen(filename, "rb");
   if (file == NULL) {
      return NULL;
   }
   
   // Compute file size
   if (fseek(file, 0, SEEK_END) == -1) {
      free(file);
      return NULL;
   }
   
   long size = ftell(file);
   if (size == -1) {
      free(file);
      return NULL;
   }
   
   // Impose a size cap to prevent disasters in case the user tries to load
   // a file that's too large (in this case we're going with the maximum
   // size that something using Echo may be able to encounter)
   if (size >= MAX_BLOBSIZE) {
      fclose(file);
      return NULL;
   }
   
   // Allocate blob large enough to hold the file
   Blob *blob = alloc_blob(size);
   
   // Load entire file into memory now
   if (fseek(file, 0, SEEK_SET) == -1) {
      fclose(file);
      free(blob);
      return NULL;
   }
   if (fread(blob->data, 1, size, file) < size) {
      fclose(file);
      free(blob);
      return NULL;
   }
   
   // We're done!
   fclose(file);
   return blob;
}

//***************************************************************************
// alloc_blob
// Allocates a new blob with the given size.
//---------------------------------------------------------------------------
// param size ... size of blob
//---------------------------------------------------------------------------
// return ....... pointer to blob
//***************************************************************************

Blob *alloc_blob(size_t size)
{
   Blob *blob = malloc(sizeof(Blob) + size);
   if (blob == NULL) abort();
   blob->size = size;
   return blob;
}

//***************************************************************************
// decode_utf8
// Decodes the UTF-8 codepoint at the beginning of the string.
//---------------------------------------------------------------------------
// param text ... pointer to string
//---------------------------------------------------------------------------
// return ....... codepoint (bad sequences are replaced with U+FFFD)
//***************************************************************************

uint32_t decode_utf8(const char *text)
{
   // To avoid dealing with the signed vs. unsigned mess :|
   // It's pretty annoying how you can't use unsigned char directly
   const uint8_t *s = (const uint8_t *)(text);
   
   // ASCII? (U+0000..U+007F)
   if (s[0] < 0x80) {
      return s[0];
   }
   
   // Two bytes? (U+0080..U+07FF)
   else if ((s[0] & 0xE0) == 0xC0) {
      if ((s[1] & 0xC0) != 0x80) return 0xFFFD;
      uint32_t code = (s[0] & 0x1F) << 6 |
                      (s[1] & 0x3F);
      if (code < 0x80) return 0xFFFD;
      return code;
   }
   
   // Three bytes? (U+0800..U+FFFD)
   else if ((s[0] & 0xF0) == 0xE0) {
      if ((s[1] & 0xC0) != 0x80) return 0xFFFD;
      if ((s[2] & 0xC0) != 0x80) return 0xFFFD;
      uint32_t code = (s[0] & 0x0F) << 12 |
                      (s[1] & 0x3F) << 6 |
                      (s[2] & 0x3F);
      if (code < 0x800) return 0xFFFD;
      if (code >= 0xD800 && code <= 0xDFFF) return 0xFFFD;
      if (code >= 0xFFFE) return 0xFFFD;
      return code;
   }
   
   // Four bytes? (U+10000..U+10FFFD)
   else if ((s[0] & 0xF8) == 0xF0) {
      if ((s[1] & 0xC0) != 0x80) return 0xFFFD;
      if ((s[2] & 0xC0) != 0x80) return 0xFFFD;
      if ((s[3] & 0xC0) != 0x80) return 0xFFFD;
      uint32_t code = (s[0] & 0x07) << 18 |
                      (s[1] & 0x3F) << 12 |
                      (s[2] & 0x3F) << 6 |
                      (s[3] & 0x3F);
      if (code < 0x10000) return 0xFFFD;
      if ((code & 0xFFFE) == 0xFFFE) return 0xFFFD;
      return code;
   }
   
   // Oops
   else {
      return 0xFFFD;
   }
}

//***************************************************************************
// advance_utf8
// Advances the string pointer until the next UTF-8 sequence.
//---------------------------------------------------------------------------
// param text ... pointer to string
//---------------------------------------------------------------------------
// return ....... updated pointer
//***************************************************************************

const char *advance_utf8(const char *text)
{
   const uint8_t *s = (const uint8_t *)(text);
   
   do {
      text++; s++;
   } while ((*s & 0xC0) == 0x80);
   
   return text;
}

//***************************************************************************
// utf8_to_utf16
//
// Turns an UTF-8 string into UTF-16 and returns a new string. You should
// call free() when you're done with the returned string.
//---------------------------------------------------------------------------
// param text ... pointer to UTF-8 string
//---------------------------------------------------------------------------
// return ....... pointer to UTF-16 string
//***************************************************************************

uint16_t *utf8_to_utf16(const char *text)
{
   // Allocate buffer
   size_t buflen = 0x80;
   size_t bufpos = 0x00;
   uint16_t *buffer = malloc(sizeof(uint16_t) * buflen);
   if (buffer == NULL) abort();
   
   for (;;) {
      // Retrieve next character
      uint32_t codepoint = decode_utf8(text);
      if (codepoint == 0) break;
      text = advance_utf8(text);
      
      // BMP codepoint?
      if (codepoint <= 0xFFFF) {
         buffer[bufpos] = codepoint;
         bufpos++;
      }
      
      // Need surrogates?
      else {
         codepoint -= 0x10000;
         buffer[bufpos+0] = 0xD800 | (codepoint & 0x3FF);
         buffer[bufpos+1] = 0xDC00 | (codepoint >> 10);
         bufpos += 2;
      }
      
      // May want to allocate more room?
      // (note: arbitrary margin here)
      if (bufpos >= (buflen - 4)) {
         buflen *= 2;
         buffer = realloc(buffer, sizeof(uint16_t) * buflen);
         if (buffer == NULL) abort();
      }
   }
   
   // We're done
   buffer[bufpos] = 0;
   return buffer;
}

//***************************************************************************
// get_utf16_size
//
// Computes the size of an UTF-16 string in *bytes* (including nul
// terminator). Intended to help figure out how much memory is needed.
//---------------------------------------------------------------------------
// param text ... pointer to string
//---------------------------------------------------------------------------
// return ....... string size in bytes (including nul terminator)
//***************************************************************************

size_t get_utf16_size(const uint16_t *text)
{
   size_t size = 0;
   
   for (;;) {
      size += 2;
      if (*text == 0) return size;
      text++;
   }
}
