
#include <stdint.h>

// Pack FT8 text message into 72 bits
// [IN] msg      - FT8 message (e.g. "CQ TE5T KN01")
// [OUT] c77     - 10 byte array to store the 77 bit payload (MSB first)
int pack77(const char *msg, uint8_t *c77);

// Pack a special token, a 22-bit hash code, or a valid base call
// into a 28-bit integer.
int32_t pack28(const char* callsign);

// Pack a special token, a 22-bit hash code, or a valid base call
// into a 28-bit integer.
int32_t pack28s(const char* callsign, int *has_suffix);

// Pack Type 1 (Standard 77-bit message) and Type 2 (ditto, with a "/P" call)
int pack77_1(const char* msg, uint8_t* b77);
