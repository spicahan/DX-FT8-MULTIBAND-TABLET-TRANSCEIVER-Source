/*
 * pack.c
 *
 *  Created on: Sep 18, 2019
 *      Author: user
 */

#include "pack.h"

#include "text.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>

static const char A0[43] = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ+-./?";
static const char A1[38] = " 0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static const char A3[11] = "0123456789";
static const char A4[28] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ";

static const char DE_[3] = "DE ";
static const char QRZ_[4] = "QRZ ";
static const char CQ_[3] = "CQ ";
static const char RRR[4] = "RRR";
static const char RR73[5] = "RR73";
static const char _73[3] = "73";
static const char SlashP[2] = "/P";
static const size_t ERROR_FLAG = 0xffffffff;

// Pack a special token, a 22-bit hash code, or a valid base call
// into a 28-bit integer.
int32_t pack28(const char* callsign)
{
    return pack28s(callsign, NULL);
}

// Pack a special token, a 22-bit hash code, or a valid base call
// into a 28-bit integer.
int32_t pack28s(const char* callsign, int *has_suffix)
{
    int32_t NTOKENS = 2063592L;
    int32_t MAX22 = 4194304L;

    if (memcmp(callsign, CQ_, sizeof(CQ_)) == 0)
        return 2;
    if (memcmp(callsign, DE_, sizeof(DE_)) == 0)
        return 0;
    if (memcmp(callsign, QRZ_, sizeof(QRZ_)) == 0)
        return 1;

    char c6[6] = "      ";

    int length = 0;
    while (callsign[length] != ' ' && callsign[length] != 0)
    {
        length++;
    }

    if (length > 3 && memcmp(callsign + length - 2, SlashP, sizeof(SlashP)) == 0)
    {
        if (has_suffix != NULL)
            *has_suffix = 1;
        length -= 2;
    }
    
    // Copy callsign intto 6 character buffer
    if (starts_with(callsign, "3DA0") && length <= 7)
    {
        // Work-around for Swaziland prefix: 3DA0XYZ -> 3D0XYZ
        memcpy(c6, "3D0", 3);
        memcpy(c6 + 3, callsign + 4, length - 4);
    }
    else if (starts_with(callsign, "3X") && is_letter(callsign[2]) && length <= 7)
    {
        // Work-around for Guinea prefixes: 3XA0XYZ -> QA0XYZ
        memcpy(c6, "Q", 1);
        memcpy(c6 + 1, callsign + 2, length - 2);
    }
    else
    {
        if (is_digit(callsign[2]) && length <= 6)
        {
            // AB0XYZ
            memcpy(c6, callsign, length);
        }
        else if (is_digit(callsign[1]) && length <= 5)
        {
            // A0XYZ -> " A0XYZ"
            memcpy(c6 + 1, callsign, length);
        }
    }

    // Check for standard callsign
    int i0, i1, i2, i3, i4, i5;
    if ((i0 = char_index(A1, c6[0])) >= 0 
        // do not match blank
        && (i1 = char_index(A1+1, c6[1])) >= 0
        && (i2 = char_index(A3, c6[2])) >= 0
        && (i3 = char_index(A4, c6[3])) >= 0
        && (i4 = char_index(A4, c6[4])) >= 0
        && (i5 = char_index(A4, c6[5])) >= 0)
    {
        // This is a standard callsign
        int32_t n28 = i0;
        n28 = n28 * 36 + i1;
        n28 = n28 * 10 + i2;
        n28 = n28 * 27 + i3;
        n28 = n28 * 27 + i4;
        n28 = n28 * 27 + i5;
        return NTOKENS + MAX22 + n28;
    }
    return -1;
}

static uint16_t packgrid(const char* grid4)
{
    uint16_t MAXGRID4 = 32400;

    if (grid4 == NULL)
    {
        // Two callsigns only, no report/grid
        return MAXGRID4 + 1;
    }

    // Take care of special cases
    if (equals(grid4, RRR))
        return MAXGRID4 + 2;
    if (equals(grid4, RR73))
        return MAXGRID4 + 3;
    if (equals(grid4, _73))
        return MAXGRID4 + 4;

    // Check for standard 4 letter grid
    if (in_range(grid4[0], 'A', 'R') 
        && in_range(grid4[1], 'A', 'R')
        && is_digit(grid4[2]) 
        && is_digit(grid4[3]))
    {
        uint16_t igrid4 = (grid4[0] - 'A');
        igrid4 = igrid4 * 18 + (grid4[1] - 'A');
        igrid4 = igrid4 * 10 + (grid4[2] - '0');
        igrid4 = igrid4 * 10 + (grid4[3] - '0');
        return igrid4;
    }

    // Parse report: +dd / -dd / R+dd / R-dd
    // TODO: check the range of dd
    if (grid4[0] == 'R')
    {
        int dd = dd_to_int(grid4 + 1, 3);
        uint16_t irpt = 35 + dd;
        return (MAXGRID4 + irpt) | 0x8000; // ir = 1
    }
    else
    {
        int dd = dd_to_int(grid4, 3);
        uint16_t irpt = 35 + dd;
        return (MAXGRID4 + irpt); // ir = 0
    }

    return MAXGRID4 + 1;
}

static size_t extra_arg_len(const char *arg_in, char *arg_out, size_t maxlen)
{
    size_t len = 0;
    while(*arg_in != ' ')
    {
        char ch = *arg_in++;
        if (ch >= 'A' && ch <= 'Z')
        {
            if (len >= maxlen)
                return ERROR_FLAG;
            *arg_out++ = ch;
            len++;
        }
        else
        {
            len = 0;
            break;
        }
    }
    return len;
}

// Pack Type 1 (Standard 77-bit message) and Type 2 (ditto, with a "/P" call)
int pack77_1(const char* msg, uint8_t* b77)
{
    int32_t n28a = -1, n28b = 0;
    if (memcmp(msg, CQ_, sizeof(CQ_)) == 0)
    {
        msg += sizeof(CQ_);
        n28a = 2;
    }
    if (memcmp(msg, DE_, sizeof(DE_)) == 0)
    {
        msg += sizeof(DE_);
        n28a = 0;
    }
    if (memcmp(msg, QRZ_, sizeof(QRZ_)) == 0)
    {
        msg += sizeof(QRZ_);
        n28a = 1;
    }

    int first_has_suffix = 0, second_has_suffix = 0;
    if (n28a == 2)
    {
        char extra_arg[5] = {0};
        size_t len = extra_arg_len(msg, extra_arg, sizeof(extra_arg) - 1);
        if (len == ERROR_FLAG)
            return -1;
        else if (len > 0) 
        {
            msg += len + 1;
            n28a = 0;
            for (size_t j = 0; j < len; j++)
            {
                n28a = n28a * 27 + extra_arg[j] - 'A' + 1;
            }
            n28a += 1003;
        }
    }

    uint16_t igrid4;
    if (n28a < 0)
    {
        n28a = pack28s(msg, &first_has_suffix);
        const char* s1 = strchr(msg, ' ');
        if (s1 == 0)
            return -1;

        n28b = pack28s(++s1, &second_has_suffix);  // second call
        s1 = strchr(s1, ' ');
        if (s1 != 0)
            s1++;
        igrid4 = packgrid(s1);
    }
    else
    {
        n28b = pack28s(msg, &second_has_suffix);  // second call
        const char *s1 = strchr(msg, ' ');
        if (s1 != 0)
            s1++;
        igrid4 = packgrid(s1);
    }

    if (n28a < 0 || n28b < 0)
        return -1;

    uint8_t i3 = 1; // Assume no suffix /P

    // Shift in ipa and ipb bits into n28a and n28b
    n28a <<= 1;
    n28b <<= 1;

    // apply the /P suffix first call
    if (first_has_suffix)
    {
        n28a += 1;
        i3 = 2;
    }

    // apply the /P suffix second call
    if (second_has_suffix)
    {
        n28b += 1;
        i3 = 2;
    }

    b77[0] = (n28a >> 21);
    b77[1] = (n28a >> 13);
    b77[2] = (n28a >> 5);
    b77[3] = (uint8_t)(n28a << 3) | (uint8_t)(n28b >> 26);
    b77[4] = (n28b >> 18);
    b77[5] = (n28b >> 10);
    b77[6] = (n28b >> 2);
    b77[7] = (uint8_t)(n28b << 6) | (uint8_t)(igrid4 >> 10);
    b77[8] = (igrid4 >> 2);
    b77[9] = (uint8_t)(igrid4 << 6) | (uint8_t)(i3 << 3);

    return 0;
}

static int packtext77(const char* text, uint8_t* b77)
{
    size_t length = strlen(text);

    // Skip leading and trailing spaces
    while (*text == ' ' && *text != 0)
    {
        ++text;
        --length;
    }

    while (length > 0 && text[length - 1] == ' ')
    {
        --length;
    }

    if (length >= 13)
    {
        return -1;
    }

    // Clear the first 72 bits representing a long number
    memset(b77, 0, 9);

    // Now express the text as base-42 number stored
    // in the first 72 bits of b77
    for (size_t j = 0; j < 13; ++j)
    {
        // Multiply the long integer in b77 by 42
        uint16_t x = 0;
        for (int i = 8; i >= 0; --i)
        {
            x += b77[i] * (uint16_t)42;
            b77[i] = (x & 0xFF);
            x >>= 8;
        }

        // Get the index of the current char
        if (j < length)
        {
            int q = char_index(A0, text[j]);
            x = (q > 0) ? q : 0;
        }
        else
        {
            x = 0;
        }

        // Here we double each added number in order to have the result multiplied
        // by two as well, so that it's a 71 bit number left-aligned in 72 bits (9 bytes)
        x <<= 1;

        // Now add the number to our long number
        for (int i = 8; i >= 0; --i)
        {
            if (x == 0)
                break;
            x += b77[i];
            b77[i] = (x & 0xFF);
            x >>= 8;
        }
    }

    // Set n3=0 (bits 71..73) and i3=0 (bits 74..76)
    b77[8] &= 0xFE;
    b77[9] &= 0x00;
    return 0;
}

int pack77(const char* msg, uint8_t* c77)
{
    // Check Type 1 (Standard 77-bit message) or Type 2, with optional "/P"
    if (pack77_1(msg, c77) == 0)
    {
        return 0;
    }

    return packtext77(msg, c77);
}
