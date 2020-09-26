#ifndef STREAM_H
#define STREAM_H

// Possible commands in a stream
typedef enum {
   STREAM_DUMMY,              // Dummy (ignore!)
   STREAM_DELAY,              // Delay
   STREAM_YMREG0,             // YM2612 register write (bank 0)
   STREAM_YMREG1,             // YM2612 register write (bank 1)
   STREAM_PSGREG,             // PSG register write
   STREAM_INITPCM,            // Set up PCM stuff
   STREAM_STARTPCM,           // Start PCM playback
   STREAM_STOPPCM,            // Stop PCM playback
   STREAM_SETPCMFREQ,         // Set PCM playback frequency
   STREAM_END,                // End of stream
} StreamCmdType;

// Data of a stream command
typedef struct {
   StreamCmdType type;        // Command to issue
   unsigned value1;           // 1st argument
   unsigned value2;           // 2nd argument
} StreamCmd;

const StreamCmd *get_stream_command(unsigned);
unsigned get_num_stream_commands(void);
unsigned get_num_stream_bytes(void);
unsigned get_num_stream_samples(void);
StreamCmd *new_command(void);
void add_delay(unsigned);
void add_ym_write(unsigned, unsigned, unsigned);
void add_psg_write(unsigned);
void setup_ym2612_pcm(void);
void start_pcm_output(unsigned);
void stop_pcm_output(void);
void set_pcm_freq(unsigned);
void end_of_stream(void);
void set_loop_point(void);
unsigned does_stream_loop(void);
unsigned get_loop_offset(void);
unsigned get_num_loop_samples(void);

#endif
