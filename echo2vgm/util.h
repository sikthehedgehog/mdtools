#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <stdio.h>

typedef struct {
   size_t size;
   uint8_t data[0];
} Blob;

char *read_line(FILE *);
Blob *load_file(const char *);
Blob *alloc_blob(size_t);
uint32_t decode_utf8(const char *);
const char *advance_utf8(const char *);
uint16_t *utf8_to_utf16(const char *);
size_t get_utf16_size(const uint16_t *);

#endif
