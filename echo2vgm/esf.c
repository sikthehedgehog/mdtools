#include <stdint.h>
#include <stdlib.h>
#include "esf.h"
#include "instruments.h"
#include "stream.h"
#include "util.h"

// Raw frequencies for all pitches
static const uint16_t fm_pitch[] = {
   644, 681, 722, 765,
   810, 858, 910, 964,
   1021,1081,1146,1214,
};
static const uint16_t psg_pitch[] = {
   851,803,758,715,675,637,601,568,536,506,477,450,
   425,401,379,357,337,318,300,284,268,253,238,225,
   212,200,189,178,168,159,150,142,134,126,119,112,
   106,100,94, 89, 84, 79, 75, 71, 67, 63, 59, 56,
   53, 50, 47, 44, 42, 39, 37, 35, 33, 31, 29, 28,
   26, 25, 23, 22, 21, 19, 18, 17, 16, 15, 14, 14,
};

// Structure used to keep FM instrument data required to
// adjust the instrument volume later
static struct {
   uint8_t algo;
   uint8_t tl_s1;
   uint8_t tl_s2;
   uint8_t tl_s3;
   uint8_t tl_s4;
} fm_data[0x08] = {
   { 0, 0x7F,0x7F,0x7F,0x7F },
   { 0, 0x7F,0x7F,0x7F,0x7F },
   { 0, 0x7F,0x7F,0x7F,0x7F },
   { 0, 0x7F,0x7F,0x7F,0x7F },
   { 0, 0x7F,0x7F,0x7F,0x7F },
   { 0, 0x7F,0x7F,0x7F,0x7F },
   { 0, 0x7F,0x7F,0x7F,0x7F },
   { 0, 0x7F,0x7F,0x7F,0x7F },
};

// PSG state
static const Blob dummy_psg = { 0 };

static struct {
   const Blob *blob;       // Instrument data
   unsigned playing;       // Set if channel is keyed on
   unsigned loop;          // Loop point for instrument
   unsigned pos;           // Current position within instrument
   unsigned vol;           // Channel volume
   unsigned base_pitch;    // Semitone-based pitch (0xFF if void)
   unsigned raw_pitch;     // Raw frequency pitch (if above void)
} psg_state[4] = {
   { &dummy_psg, 0,0,0,0,0xFF,0 },
   { &dummy_psg, 0,0,0,0,0xFF,0 },
   { &dummy_psg, 0,0,0,0,0xFF,0 },
   { &dummy_psg, 0,0,0,0,0xFF,0 },
};

// Array to keep track of unhandled events we already warned about
// Otherwise it becomes a spam of warnings x_x
static uint8_t warned[0x100] = { 0 };

// Private functions
static void key_on_fm(unsigned, unsigned);
static void key_on_psg(unsigned, unsigned);
static void key_on_noise(unsigned);
static void key_on_pcm(unsigned);
static void key_off_fm(unsigned);
static void key_off_psg(unsigned);
static void key_off_pcm(void);
static void set_fm_volume(unsigned, unsigned);
static void set_psg_volume(unsigned, unsigned);
static void set_fm_pitch(unsigned, unsigned);
static void set_fm_raw_pitch(unsigned, unsigned);
static void set_psg_pitch(unsigned, unsigned);
static void set_psg_raw_pitch(unsigned, unsigned);
static void set_fm_params(unsigned, unsigned);
static void load_fm_instrument(unsigned, unsigned);
static void load_psg_instrument(unsigned, unsigned);
static void do_echo_loop(unsigned);
static void warn_about(uint8_t);

//***************************************************************************
// parse_esf
// Loads and parses an ESF file and generates the relevant stream commands.
//---------------------------------------------------------------------------
// param filename ... ESF filename
//---------------------------------------------------------------------------
// return ........... 0 on success, -1 on failure
//***************************************************************************

int parse_esf(const char *esfname)
{
   // Some initialization
   setup_ym2612_pcm();
   set_pcm_freq(10650);
   
   add_ym_write(0,0x28, 0x00);
   add_ym_write(0,0x28, 0x01);
   add_ym_write(0,0x28, 0x02);
   add_ym_write(0,0x28, 0x04);
   add_ym_write(0,0x28, 0x05);
   add_ym_write(0,0x28, 0x06);
   
   add_ym_write(0,0xB4, 0xC0);
   add_ym_write(0,0xB5, 0xC0);
   add_ym_write(0,0xB6, 0xC0);
   add_ym_write(1,0xB4, 0xC0);
   add_ym_write(1,0xB5, 0xC0);
   add_ym_write(1,0xB6, 0xC0);
   
   add_ym_write(0,0x2A, 0x80);
   add_ym_write(0,0x2B, 0x00);
   
   // Load entire ESF file into memory
   Blob *blob = load_file(esfname);
   if (blob == NULL) {
      fprintf(stderr, "Error: can't open ESF file \"%s\"\n", esfname);
      return -1;
   }
   
   // Scan through whole file
   unsigned i;
   for (i = 0; i < blob->size; )
   switch (blob->data[i]) {
      // Key-on
      case 0x00: case 0x01: case 0x02:
      case 0x04: case 0x05: case 0x06: {
         if ((blob->size - i) < 2) goto error;
         key_on_fm(blob->data[i+0] & 0x07,
                   blob->data[i+1]);
         i += 2;
      } break;
      
      case 0x08: case 0x09: case 0x0A: {
         if ((blob->size - i) < 2) goto error;
         key_on_psg(blob->data[i+0] & 0x03,
                    blob->data[i+1]);
         i += 2;
      } break;
      
      case 0x0B: {
         if ((blob->size - i) < 2) goto error;
         key_on_noise(blob->data[i+1]);
         i += 2;
      } break;
      
      case 0x0C: {
         if ((blob->size - i) < 2) goto error;
         key_on_pcm(blob->data[i+1]);
         i += 2;
      } break;
      
      // Key-off
      case 0x10: case 0x11: case 0x12:
      case 0x14: case 0x15: case 0x16: {
         key_off_fm(blob->data[i] & 0x07);
         i++;
      } break;
      
      case 0x18: case 0x19: case 0x1A: case 0x1B: {
         key_off_psg(blob->data[i] & 0x03);
         i++;
      } break;
      
      case 0x1C: {
         key_off_pcm();
         i++;
      } break;
      
      // Set volume
      case 0x20: case 0x21: case 0x22:
      case 0x24: case 0x25: case 0x26: {
         if ((blob->size - i) < 2) goto error;
         set_fm_volume(blob->data[i+0] & 0x07,
                       blob->data[i+1]);
         i += 2;
      } break;
      
      case 0x28: case 0x29: case 0x2A: case 0x2B: {
         if ((blob->size - i) < 2) goto error;
         set_psg_volume(blob->data[i+0] & 0x03,
                        blob->data[i+1]);
         i += 2;
      } break;
      
      // Set frequency
      case 0x30: case 0x31: case 0x32:
      case 0x34: case 0x35: case 0x36: {
         if ((blob->size - i) < 2) goto error;
         
         if (blob->data[i+1] & 0x80) {
            set_fm_pitch(blob->data[i+0] & 0x07,
                         blob->data[i+1]);
            i += 2;
         } else {
            if ((blob->size - i) < 3) goto error;
            set_fm_raw_pitch(blob->data[i+0] & 0x07,
                             blob->data[i+1] << 8 |
                             blob->data[i+2]);
            i += 3;
         }
      } break;
      
      case 0x38: case 0x39: case 0x3A: {
         if ((blob->size - i) < 2) goto error;
         
         if (blob->data[i+1] & 0x80) {
            set_psg_pitch(blob->data[i+0] & 0x03,
                          blob->data[i+1]);
            i += 2;
         } else {
            if ((blob->size - i) < 3) goto error;
            set_psg_raw_pitch(blob->data[i+0] & 0x03,
                             (blob->data[i+1] & 0x0F) |
                             (blob->data[i+2] << 4));
            i += 3;
         }
      } break;
      
      case 0x3B: {
         if ((blob->size - i) < 2) goto error;
         set_psg_pitch(3, blob->data[i+1]);
         i += 2;
      } break;
      
      // FM parameters
      case 0xF0: case 0xF1: case 0xF2:
      case 0xF4: case 0xF5: case 0xF6: {
         if ((blob->size - i) < 2) goto error;
         set_fm_params(blob->data[i+0] & 0x07,
                       blob->data[i+1]);
         i += 2;
      } break;
      
      // Load instrument
      case 0x40: case 0x41: case 0x42:
      case 0x44: case 0x45: case 0x46: {
         if ((blob->size - i) < 2) goto error;
         load_fm_instrument(blob->data[i+0] & 0x07,
                            blob->data[i+1]);
         i += 2;
      } break;
      
      case 0x48: case 0x49: case 0x4A: case 0x4B: {
         if ((blob->size - i) < 2) goto error;
         load_psg_instrument(blob->data[i+0] & 0x03,
                             blob->data[i+1]);
         i += 2;
      } break;
      
      // Direct register writes
      case 0xF8: case 0xF9: {
         if ((blob->size - i) < 3) goto error;
         add_ym_write(blob->data[i+0] & 0x01,
                      blob->data[i+1],
                      blob->data[i+2]);
         i += 3;
      } break;
      
      // Flag commands (ignore!)
      case 0xFA: case 0xFB: {
         if ((blob->size - i) < 2) goto error;
         i += 2;
      } break;
      
      // Delay
      case 0xD0: case 0xD1: case 0xD2: case 0xD3:
      case 0xD4: case 0xD5: case 0xD6: case 0xD7:
      case 0xD8: case 0xD9: case 0xDA: case 0xDB:
      case 0xDC: case 0xDD: case 0xDE: case 0xDF: {
         do_echo_loop((blob->data[i] & 0x0F) + 1);
         i++;
      } break;
      
      case 0xFE: {
         if ((blob->size - i) < 2) goto error;
         if (blob->data[i+1] != 0x00) {
            do_echo_loop(blob->data[i+1]);
         } else {
            do_echo_loop(0x100);
         }
         i += 2;
      } break;
      
      // Channel lock commands (ignore!)
      case 0xE0: case 0xE1: case 0xE2: case 0xE3:
      case 0xE4: case 0xE5: case 0xE6: case 0xE7:
      case 0xE8: case 0xE9: case 0xEA: case 0xEB:
      case 0xEC: case 0xED: case 0xEE: case 0xEF: {
         i++;
      } break;
      
      // Set loop point
      case 0xFD: {
         set_loop_point();
         i++;
      } break;
      
      // Loop stream or end of stream
      case 0xFC: case 0xFF: {
         i = blob->size;
      } break;
      
      // Unhandled Echo event!
      default: {
         fprintf(stderr,
                 "[INTERNAL] Error: unhandled Echo event $%02X!\n",
                 blob->data[i]);
         for (unsigned count = 0; count < 16; count++, i++) {
            if (i >= blob->size) break;
            fprintf(stderr, "%02X ", blob->data[i]);
         }
         fprintf(stderr, "\n");
         free(blob);
         return -1;
      } break;
   }
   
   // We're done
   end_of_stream();
   free(blob);
   return 0;
   
error:
   fprintf(stderr, "Error: invalid ESF file \"%s\"\n", esfname);
   free(blob);
   return -1;
}

//***************************************************************************
// key_on_fm [private]
// Issues a key-on for a FM channel.
//---------------------------------------------------------------------------
// param chan .... FM channel (0,1,2,4,5,6)
// param pitch ... pitch to play at (see ESF spec)
//***************************************************************************

static void key_on_fm(unsigned chan, unsigned pitch)
{
   // Release all operators
   add_ym_write(0, 0x28, chan);
   
   // Pitch is stored in a weird way
   pitch >>= 1;
   unsigned semitone = pitch & 0x0F;
   unsigned octave = pitch >> 4;
   
   // Set up frequency
   uint16_t raw = fm_pitch[semitone] | (octave << 11);
   unsigned bank = chan >> 2;
   unsigned base = chan & 0x03;
   
   add_ym_write(bank, 0xA4+base, (uint8_t)(raw >> 8));
   add_ym_write(bank, 0xA0+base, (uint8_t)(raw));
   
   // Attack all operators
   add_ym_write(0, 0x28, 0xF0|chan);
}

//***************************************************************************
// key_on_psg [private]
// Issues a key-on for a square wave PSG channel.
//---------------------------------------------------------------------------
// param chan .... PSG channel (0..2)
// param pitch ... pitch to play at (see ESF spec)
//***************************************************************************

static void key_on_psg(unsigned chan, unsigned pitch)
{
   // Set up channel to restart instrument
   psg_state[chan].playing = 1;
   psg_state[chan].loop = 0;
   psg_state[chan].pos = 0;
   
   // Store pitch (note ESF stores it in a weird way)
   psg_state[chan].base_pitch = pitch >> 1;
}

//***************************************************************************
// key_on_noise [private]
// Issues a key-on for the noise PSG channel.
//---------------------------------------------------------------------------
// param noise ... noise type (0..7)
//***************************************************************************

static void key_on_noise(unsigned noise)
{
   // Set up channel to restart instrument
   psg_state[3].playing = 1;
   psg_state[3].loop = 0;
   psg_state[3].pos = 0;
   
   // Store noise
   psg_state[3].base_pitch = noise;
}

//***************************************************************************
// key_on_pcm
// Plays a PCM instrument.
//---------------------------------------------------------------------------
// param id ... instrument ID
//***************************************************************************

static void key_on_pcm(unsigned id)
{
   // Retrieve PCM properties (allocates the instrument into the PCM block
   // too if it wasn't there already, otherwise reuses it)
   unsigned pcm_id = get_pcm_id(id);
   
   // Enable DAC
   add_ym_write(0,0x2A,0x80);
   add_ym_write(0,0x2B,0x80);
   
   // Start stream
   start_pcm_output(pcm_id);
}

//***************************************************************************
// key_off_fm [private]
// Issues a key-off for a FM channel.
//---------------------------------------------------------------------------
// param chan ... FM channel (0,1,2,4,5,6)
//***************************************************************************

static void key_off_fm(unsigned chan)
{
   // Release all operators
   add_ym_write(0, 0x28, chan);
}

//***************************************************************************
// key_off_psg [private]
// Issues a key-off for a PSG channel.
//---------------------------------------------------------------------------
// param chan ... PSG channel (0..3)
//***************************************************************************

static void key_off_psg(unsigned chan)
{
   psg_state[chan].playing = 0;
}

//***************************************************************************
// key_off_pcm [private]
// Issues a key-off for the PCM channel.
//***************************************************************************

static void key_off_pcm(void)
{
   // Stop PCM playback
   stop_pcm_output();
   
   // Disable DAC
   add_ym_write(0,0x2B,0x00);
   add_ym_write(0,0x2A,0x80);
}

//***************************************************************************
// set_fm_volume [private]
// Adjusts the volume of a FM channel.
//---------------------------------------------------------------------------
// param chan ... FM channel (0,1,2,4,5,6)
// param vol .... volume (0 = loudest, 127 = quietest)
//***************************************************************************

static void set_fm_volume(unsigned chan, unsigned vol)
{
   // Determine bank and base register
   unsigned bank = chan >> 2;
   unsigned reg = 0x40 + (chan & 0x03);
   
   // Adjust S1 if needed (only algorithm 7)
   if (fm_data[chan].algo == 7) {
      unsigned tl = fm_data[chan].tl_s1 + vol;
      if (tl > 0x7F) tl = 0x7F;
      add_ym_write(bank, reg+0x00, tl);
   }
   
   // Adjust S3 if needed (algorithms 5 and above)
   if (fm_data[chan].algo >= 5) {
      unsigned tl = fm_data[chan].tl_s3 + vol;
      if (tl > 0x7F) tl = 0x7F;
      add_ym_write(bank, reg+0x04, tl);
   }
   
   // Adjust S2 if needed (algorithms 4 and above)
   if (fm_data[chan].algo >= 4) {
      unsigned tl = fm_data[chan].tl_s2 + vol;
      if (tl > 0x7F) tl = 0x7F;
      add_ym_write(bank, reg+0x08, tl);
   }
   
   // Adjust S4 (all algorithms)
   {
      unsigned tl = fm_data[chan].tl_s4 + vol;
      if (tl > 0x7F) tl = 0x7F;
      add_ym_write(bank, reg+0x0C, tl);
   }
}

//***************************************************************************
// set_psg_volume [private]
// Adjust the volume of a PSG channel.
//---------------------------------------------------------------------------
// param chan ... PSG channel (0..3)
// param vol .... volume (0 = loudest, 15 = quietest)
//***************************************************************************

static void set_psg_volume(unsigned chan, unsigned vol)
{
   psg_state[chan].vol = vol;
}

//***************************************************************************
// set_fm_pitch
// Slides a FM channel's pitch, measured in semitones
//---------------------------------------------------------------------------
// param chan .... FM channel (0,1,2,4,5,6)
// param pitch ... pitch byte (see ESF spec)
//***************************************************************************

static void set_fm_pitch(unsigned chan, unsigned pitch)
{
   // Split pitch into its parts
   pitch &= 0x7F;
   unsigned octave = pitch >> 4;
   unsigned semitone = pitch & 0x0F;
   
   // Set up frequency
   uint16_t raw = fm_pitch[semitone] | (octave << 11);
   unsigned bank = chan >> 2;
   unsigned base = chan & 0x03;
   
   add_ym_write(bank, 0xA4+base, (uint8_t)(raw >> 8));
   add_ym_write(bank, 0xA0+base, (uint8_t)(raw));
}

//***************************************************************************
// set_fm_raw_pitch
// Slides a FM channel's pitch, given as raw frequency
//---------------------------------------------------------------------------
// param chan .... FM channel (0,1,2,4,5,6)
// param pitch ... frequency (YM2612 format)
//***************************************************************************

static void set_fm_raw_pitch(unsigned chan, unsigned freq)
{
   unsigned bank = chan >> 2;
   unsigned base = chan & 0x03;
   
   add_ym_write(bank, 0xA4+base, (uint8_t)(freq >> 8));
   add_ym_write(bank, 0xA0+base, (uint8_t)(freq));
}

//***************************************************************************
// set_psg_pitch
// Slides a PSG channel's pitch, measured in semitones (also works for noise)
//---------------------------------------------------------------------------
// param chan .... PSG channel (0..3)
// param pitch ... pitch byte (see ESF spec)
//***************************************************************************

static void set_psg_pitch(unsigned chan, unsigned pitch)
{
   pitch &= 0x7F;
   psg_state[chan].base_pitch = pitch;
}

//***************************************************************************
// set_psg_raw_pitch
// Slides a square PSG channel's pitch by specifying the raw frequency.
//---------------------------------------------------------------------------
// param chan ... PSG channel (0..2)
// param freq ... raw frequency (PSG format)
//***************************************************************************

static void set_psg_raw_pitch(unsigned chan, unsigned freq)
{
   psg_state[chan].base_pitch = 0xFF;
   psg_state[chan].raw_pitch = freq;
}

//***************************************************************************
// set_fm_params
// Changes panning, PMS and AMS of a FM channel.
//---------------------------------------------------------------------------
// param chan ..... FM channel (0,1,2,4,5,6)
// param params ... FM parameters (see ESF spec)
//***************************************************************************

static void set_fm_params(unsigned chan, unsigned params)
{
   unsigned bank = chan >> 2;
   unsigned reg = 0xB4 | (chan & 0x03);
   add_ym_write(bank, reg, params);
}

//***************************************************************************
// load_fm_instrument [private]
// Generates a load FM instrument event.
//---------------------------------------------------------------------------
// param chan ... FM channel (0,1,2,4,5,6)
// param id ..... instrument ID (0..255)
//***************************************************************************

static void load_fm_instrument(unsigned chan, unsigned id)
{
   // Order in which the YM2612 registers are stored
   static const uint8_t format[] = {
      0xB0,
      0x30,0x34,0x38,0x3C,
      0x40,0x44,0x48,0x4C,
      0x50,0x54,0x58,0x5C,
      0x60,0x64,0x68,0x6C,
      0x70,0x74,0x78,0x7C,
      0x80,0x84,0x88,0x8C,
      0x90,0x94,0x98,0x9C,
   };
   
   // Release all operators
   add_ym_write(0, 0x28, chan);
   
   // Retrieve instrument
   const Blob *blob = get_instrument(id);
   if (blob->size != 29) return;
   
   // Store FM instrument data we need for adjusting volume
   fm_data[chan].algo = blob->data[0] & 0x07;
   fm_data[chan].tl_s1 = blob->data[5] & 0x7F;
   fm_data[chan].tl_s3 = blob->data[6] & 0x7F;
   fm_data[chan].tl_s2 = blob->data[7] & 0x7F;
   fm_data[chan].tl_s4 = blob->data[8] & 0x7F;
   
   // Write YM2612 registers
   unsigned bank = chan >> 2;
   unsigned base = chan & 0x03;
   
   for (unsigned i = 0; i < 29; i++) {
      add_ym_write(bank, format[i] + base, blob->data[i]);
   }
}

//***************************************************************************
// load_psg_instrument [private]
// Generates a load PSG instrument event.
//---------------------------------------------------------------------------
// param chan ... PSG channel (0..3)
// param id ..... instrument ID (0..255)
//***************************************************************************

static void load_psg_instrument(unsigned chan, unsigned id)
{
   // Reset channel
   psg_state[chan].playing = 0;
   
   // Retrieve instrument
   psg_state[chan].blob = get_instrument(id);
}

//***************************************************************************
// do_echo_loop [private]
// Simulates Echo's idle loop whenever delays come in.
//---------------------------------------------------------------------------
// param ticks ... number of ticks to run
//***************************************************************************

static void do_echo_loop(unsigned ticks)
{
   // Pitch offsets for square wave PSG instruments
   static const signed pitch_offset[] = {
      0,                      // 0x
      +1,+2,+3,+4,+6,+8,+12,  // 1x..7x
      -1,-2,-3,-4,-6,-8,-12,  // 8x..Ex
   };
   
   // Process all iterations
   while (ticks > 0) {
      ticks--;
      
      // Process all PSG channels
      for (unsigned chan = 0; chan <= 3; chan++) {
         // Muted?
         if (!psg_state[chan].playing) {
            add_psg_write(0x9F | (chan << 5));
            continue;
         }
         
         // Process instrument
         const Blob *blob = psg_state[chan].blob;
         unsigned pos = psg_state[chan].pos;
         unsigned vol = psg_state[chan].vol;
         
         unsigned instr_vol = 0x0F;
         unsigned instr_pitch = 0;
         
         for (unsigned done = 0; !done; ) {
            // Oops?
            if (blob == NULL) {
               done = 1;
               instr_vol = 0x0F;
               break;
            }
            if (pos >= blob->size) {
               done = 1;
               instr_vol = 0x0F;
               break;
            }
            
            // Process next byte
            switch(blob->data[pos]) {
               // Set loop point
               case 0xFE:
                  psg_state[chan].loop = pos;
                  pos++;
                  break;
               
               // Go to loop point
               case 0xFF:
                  pos = psg_state[chan].loop;
                  break;
               
               // Envelope data
               default:
                  if (blob->data[pos] >= 0xF0) { done = 1; break; }
                  instr_vol = blob->data[pos] & 0x0F;
                  instr_pitch = pitch_offset[blob->data[pos] >> 4];
                  pos++;
                  done = 1;
                  break;
            }
         }
         
         psg_state[chan].pos = pos;
         
         // Compute volume
         unsigned final_vol = vol + instr_vol;
         if (final_vol > 0x0F) final_vol = 0x0F;
         add_psg_write(0x90 | (chan << 5) | final_vol);
         
         // Square wave?
         if (chan != 3) {
            // Compute final pitch
            unsigned final_freq;
            if (psg_state[chan].base_pitch != 0xFF) {
               unsigned pitch = psg_state[chan].base_pitch + instr_pitch;
               if (pitch >= 72) final_freq = 0;
               else final_freq = psg_pitch[pitch];
            } else {
               final_freq = psg_state[chan].raw_pitch;
            }
            
            // Set up channel pitch
            add_psg_write(0x80 | (chan << 5) | (final_freq & 0x0F));
            add_psg_write(final_freq >> 4);
         }
         
         // Noise
         else {
            // Play noise as-is
            add_psg_write(0xE0 | psg_state[3].base_pitch);
         }
      }
      
      // Wait for next frame
      add_delay(735);
   }
}

//***************************************************************************
// warn_about [private]
// Warns about skipped but unimplemented events. It makes sure to only warn
// about each skipped event type only once if it shows up multiple times.
//---------------------------------------------------------------------------
// param type ... first byte of event
//***************************************************************************

static void warn_about(uint8_t type)
{
   // Don't warn multiple times
   if (warned[type]) return;
   warned[type] = 1;
   
   // Issue warning
   fprintf(stderr, "[INTERNAL] Warning: skipped Echo event $%02X!\n", type);
}
