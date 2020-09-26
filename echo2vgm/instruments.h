#ifndef INSTRUMENTS_H
#define INSTRUMENTS_H

#include "util.h"

int load_instruments(const char *);
const Blob *get_instrument(unsigned);
void mark_as_pcm(unsigned);
unsigned get_pcm_id(unsigned);
const Blob *get_pcm_blob(void);

#endif
