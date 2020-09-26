#ifndef GD3_H
#define GD3_H

#include "util.h"

void set_track_title(const char *);
void set_game_title(const char *);
void set_composer(const char *);
void set_release(const char *);
void set_rippedby(const char *);
void compile_gd3(void);
const Blob *get_gd3_blob(void);

#endif
