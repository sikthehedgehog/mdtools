#ifndef STREAM_H
#define STREAM_H

// Required headers
#include <stddef.h>
#include <stdint.h>

// Possible types of event
typedef enum {
   EV_NOP,                    // No-op, used to mess with timestamps
   EV_NOTEON,                 // Note on
   EV_NOTEOFF,                // Note off
   EV_SETNOTE,                // Set semitone
   EV_SETFREQ,                // Set frequency
   EV_SETVOL,                 // Set volume
   EV_SETPAN,                 // Set panning
   EV_SETINSTR,               // Set instrument
   EV_SETTEMPO,               // Set tempo
   EV_SETREG,                 // Set register
   EV_FLAGS,                  // Change flags
   EV_LOCK,                   // Lock channel
   EV_LOOP,                   // Loop point
} EventType;

// What's in an event
typedef struct {
   uint64_t timestamp;        // Timestamp in ticks
   unsigned channel;          // Channel it belongs to
   EventType type;            // What type of event is it?
   unsigned value;            // Event parameter as needed
} Event;

// Function prototypes
void add_nop(uint64_t);
void add_note_on(uint64_t, unsigned, unsigned);
void add_note_off(uint64_t, unsigned);
void add_set_note(uint64_t, unsigned, unsigned);
void add_set_freq(uint64_t, unsigned, unsigned, unsigned);
void add_set_vol(uint64_t, unsigned, unsigned);
void add_set_pan(uint64_t, unsigned, unsigned);
void add_set_instr(uint64_t, unsigned, unsigned);
void add_set_tempo(uint64_t, unsigned);
void add_set_reg(uint64_t, unsigned, unsigned);
void add_set_flags(uint64_t, unsigned, unsigned);
void add_lock(uint64_t, unsigned);
void add_loop(uint64_t);
void sort_stream(void);
size_t get_num_events(void);
const Event *get_event(size_t);

#endif
