#include <stdlib.h>
#include "stream.h"

static StreamCmd *commands = NULL;        // List of commands
static unsigned num_commands = 0;         // Number of commands
static unsigned stream_size = 0;          // Estimated size in bytes
static unsigned stream_samples = 0;       // Estimated size in samples

static unsigned has_loop = 0;             // Set if stream loops
static unsigned loop_offset = 0;          // Loop point in bytes
static unsigned loop_samples = 0;         // Loop point in samples

//***************************************************************************
// get_stream_command
// Retrieve a command from a stream.
//---------------------------------------------------------------------------
// param id ... command ID within stream
//---------------------------------------------------------------------------
// return ..... pointer to command
//***************************************************************************

const StreamCmd *get_stream_command(unsigned id)
{
   return &commands[id];
}

//***************************************************************************
// get_num_stream_commands
// Get number of commands in the stream.
//---------------------------------------------------------------------------
// return ... number of commands in the stream
//***************************************************************************

unsigned get_num_stream_commands(void)
{
   return num_commands;
}

//***************************************************************************
// get_num_stream_bytes
// Get length of stream in bytes.
//---------------------------------------------------------------------------
// return ... number of samples in the stream
//***************************************************************************

unsigned get_num_stream_bytes(void)
{
   return stream_size;
}

//***************************************************************************
// get_num_stream_samples
// Get length of stream in samples.
//---------------------------------------------------------------------------
// return ... number of samples in the stream
//***************************************************************************

unsigned get_num_stream_samples(void)
{
   return stream_samples;
}

//***************************************************************************
// new_command
// Allocates a new command in the stream.
//---------------------------------------------------------------------------
// return: pointer to new command
//***************************************************************************

StreamCmd *new_command(void)
{
   // Allocate new entry
   num_commands++;
   StreamCmd *temp = realloc(commands, sizeof(StreamCmd) * num_commands);
   if (temp == NULL) abort();
   commands = temp;
   
   // Initialize command and return it
   StreamCmd *ptr = &commands[num_commands-1];
   ptr->type = 0;
   ptr->value1 = 0;
   ptr->value2 = 0;
   return ptr;
}

//***************************************************************************
// add_delay
// Inserts a delay into the stream.
//---------------------------------------------------------------------------
// param ticks ... number of samples to wait (at 44100Hz)
//***************************************************************************

void add_delay(unsigned samples)
{
   // A delay command can't handle longer than 65535 samples
   // Split those into multiple delays
   while (samples > 65535) {
      add_delay(65535);
      samples -= 65535;
   }
   
   // No samples to wait? Do nothing then
   if (samples == 0) {
      return;
   }
   
   // Insert command
   StreamCmd *cmd = new_command();
   cmd->type = STREAM_DELAY;
   cmd->value1 = samples;
   
   // Delays are three bytes long (61 nn nn)
   stream_size += 3;
   stream_samples += samples;
}

//***************************************************************************
// add_ym_write
// Inserts a YM2612 register write command into the stream.
//---------------------------------------------------------------------------
// param bank .... YM2612 bank (0..1)
// param reg ..... YM2612 register (0..255)
// param value ... value to write (0..255)
//***************************************************************************

void add_ym_write(unsigned bank, unsigned reg, unsigned value)
{
   // Insert command
   StreamCmd *cmd = new_command();
   cmd->type = bank ? STREAM_YMREG1 : STREAM_YMREG0;
   cmd->value1 = reg;
   cmd->value2 = value;
   
   // YM2612 register writes are three bytes long
   // For bank 0: 52 rr nn
   // For bank 1: 53 rr nn
   stream_size += 3;
}

//***************************************************************************
// add_psg_write
// Inserts a PSG register write command into the stream.
//---------------------------------------------------------------------------
// param value ... value to write (0..255)
//***************************************************************************

void add_psg_write(unsigned value)
{
   // Insert command
   StreamCmd *cmd = new_command();
   cmd->type = STREAM_PSGREG;
   cmd->value1 = value;
   
   // PSG register writes are two bytes long (50 nn)
   stream_size += 2;
}

//***************************************************************************
// setup_ym2612_pcm
// Inserts the commands that set up the PCM stream for the YM2612.
//***************************************************************************

void setup_ym2612_pcm(void)
{
   // Insert command
   StreamCmd *cmd = new_command();
   cmd->type = STREAM_INITPCM;
   
   // The setup is ten bytes long
   // 90 00 02 00 0A  91 00 00 01 00
   stream_size += 10;
}

//***************************************************************************
// start_pcm_output
// Inserts command to start streaming PCM data to YM2612 DAC.
//---------------------------------------------------------------------------
// param id ... PCM block ID
//***************************************************************************

void start_pcm_output(unsigned id)
{
   // Insert command
   StreamCmd *cmd = new_command();
   cmd->type = STREAM_STARTPCM;
   cmd->value1 = id;
   
   // PCM stream start is five bytes
   // (95 00 ii ii 00)
   stream_size += 5;
}

//***************************************************************************
// stop_pcm_output
// Inserts command to stop streaming PCM data to YM2612 DAC.
//***************************************************************************

void stop_pcm_output(void)
{
   // Insert command
   StreamCmd *cmd = new_command();
   cmd->type = STREAM_STOPPCM;
   
   // PCM stream stop is two bytes long (94 00)
   stream_size += 2;
}

//***************************************************************************
// set_pcm_freq
// Sets the PCM playback sample rate.
//---------------------------------------------------------------------------
// param hz ... PCM sample rate (in hertz)
//***************************************************************************

void set_pcm_freq(unsigned hz)
{
   // Insert command
   StreamCmd *cmd = new_command();
   cmd->type = STREAM_SETPCMFREQ;
   cmd->value1 = hz;
   
   // PCM stream stop is six bytes long (92 00 nn nn nn nn)
   stream_size += 6;
}

//***************************************************************************
// end_of_stream
// Inserts the command that finishes the stream.
//***************************************************************************

void end_of_stream(void)
{
   // Insert command
   StreamCmd *cmd = new_command();
   cmd->type = STREAM_END;
   
   // This is a single byte command (66)
   stream_size++;
}

//***************************************************************************
// set_loop_point
// Sets the stream's loop point to the current position.
//***************************************************************************

void set_loop_point(void)
{
   has_loop = 1;
   loop_offset = stream_size;
   loop_samples = stream_samples;
}

//***************************************************************************
// does_stream_loop
// Returns if a stream loops or not.
//---------------------------------------------------------------------------
// return ... 0 if it doesn't loop, 1 if it does loop
//***************************************************************************

unsigned does_stream_loop(void)
{
   return has_loop;
}

//***************************************************************************
// get_loop_offset
// Gets the offset to the loop begin in samples.
//---------------------------------------------------------------------------
// return ... offset to loop point
//***************************************************************************

unsigned get_loop_offset(void)
{
   return loop_offset;
}

//***************************************************************************
// get_num_loop_samples
// Gets how long the loop is in samples.
//---------------------------------------------------------------------------
// return ... loop length in samples
//***************************************************************************

unsigned get_num_loop_samples(void)
{
   return stream_samples - loop_samples;
}
