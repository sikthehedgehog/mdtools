#ifndef MAIN_H
#define MAIN_H

// Allowed ROM size limits
#define MIN_ROM_SIZE 0x200
#define MAX_ROM_SIZE 0x400000

// Location of header fields
#define HEADER_COPYRIGHT      0x113    // Copyright code
#define HEADER_DATE           0x118    // Build date
#define HEADER_TITLE1         0x120    // Domestic title
#define HEADER_TITLE2         0x150    // Overseas title
#define HEADER_CHECKSUM       0x18E    // Checksum
#define HEADER_SERIALNO       0x183    // Serial number
#define HEADER_REVISION       0x18C    // Revision
#define PROGRAM_START         0x200    // Where game code proper starts

// Other limits
#define DATE_LEN              8        // Length of build date field
#define TITLE_LEN             48       // Length of title fields
#define COPYRIGHT_LEN         4        // Length of copyright code
#define SERIALNO_LEN          8        // Length of serial number

typedef struct {
   size_t size;                        // Size in bytes
   uint8_t blob[MAX_ROM_SIZE];         // Where ROM is stored
} Rom;

// ROM padding modes
typedef enum {
   PADDING_QUIET,                      // Just pad
   PADDING_VERBOSE,                    // Pad and report sizes
} PadMode;

#endif
