/*
 * MPEG Audio decoder
 * copyright (c) 2002 Fabrice Bellard
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file
 * mpeg audio layer decoder tables.
 */

#include <stddef.h>
#include <stdint.h>

#include "libavutil/avassert.h"
#include "libavutil/libm.h"
#include "libavutil/thread.h"

#include "mpegaudiodata.h"

#include "mpegaudiodec_common_tablegen.h"

uint16_t ff_scale_factor_modshift[64];

static int16_t division_tab3[1 << 6 ];
static int16_t division_tab5[1 << 8 ];
static int16_t division_tab9[1 << 11];

int16_t *const ff_division_tabs[4] = {
    division_tab3, division_tab5, NULL, division_tab9
};


/*******************************************************/
/* layer 3 tables */

const uint8_t ff_slen_table[2][16] = {
    { 0, 0, 0, 0, 3, 1, 1, 1, 2, 2, 2, 3, 3, 3, 4, 4 },
    { 0, 1, 2, 3, 0, 1, 2, 3, 1, 2, 3, 1, 2, 3, 2, 3 },
};

const uint8_t ff_lsf_nsf_table[6][3][4] = {
    { {  6,  5,  5, 5 }, {  9,  9,  9, 9 }, {  6,  9,  9, 9 } },
    { {  6,  5,  7, 3 }, {  9,  9, 12, 6 }, {  6,  9, 12, 6 } },
    { { 11, 10,  0, 0 }, { 18, 18,  0, 0 }, { 15, 18,  0, 0 } },
    { {  7,  7,  7, 0 }, { 12, 12, 12, 0 }, {  6, 15, 12, 0 } },
    { {  6,  6,  6, 3 }, { 12,  9,  9, 6 }, {  6, 12,  9, 6 } },
    { {  8,  8,  5, 0 }, { 15, 12,  9, 0 }, {  6, 18,  9, 0 } },
};

/* mpegaudio layer 3 huffman tables */
VLC ff_huff_vlc[16];
static VLCElem huff_vlc_tables[128 + 128 + 128 + 130 + 128 + 154 + 166 + 142 +
                               204 + 190 + 170 + 542 + 460 + 662 + 414];
VLC ff_huff_quad_vlc[2];
static VLCElem huff_quad_vlc_tables[64 + 16];

static const uint8_t mpa_hufflens[] = {
    /* Huffman table 1 - 4 entries */
     3,  3,  2,  1,
    /* Huffman table 2 - 9 entries */
     6,  6,  5,  5,  5,  3,  3,  3,  1,
    /* Huffman table 3 - 9 entries */
     6,  6,  5,  5,  5,  3,  2,  2,  2,
    /* Huffman table 5 - 16 entries */
     8,  8,  7,  6,  7,  7,  7,  7,  6,  6,  6,  6,  3,  3,  3,  1,
    /* Huffman table 6 - 16 entries */
     7,  7,  6,  6,  6,  5,  5,  5,  5,  4,  4,  4,  3,  2,  3,  3,
    /* Huffman table 7 - 36 entries */
    10, 10, 10, 10,  9,  9,  9,  9,  8,  8,  9,  9,  8,  9,  9,  8,  8,  7,  7,
     7,  8,  8,  8,  8,  7,  7,  7,  7,  6,  5,  6,  6,  4,  3,  3,  1,
    /* Huffman table 8 - 36 entries */
    11, 11, 10,  9, 10, 10,  9,  9,  9,  8,  8,  9,  9,  9,  9,  8,  8,  8,  7,
     8,  8,  8,  8,  8,  8,  8,  8,  6,  6,  6,  4,  4,  2,  3,  3,  2,
    /* Huffman table 9 - 36 entries */
     9,  9,  8,  8,  9,  9,  8,  8,  8,  8,  7,  7,  7,  8,  8,  7,  7,  7,  7,
     6,  6,  6,  6,  5,  5,  6,  6,  5,  5,  4,  4,  4,  3,  3,  3,  3,
    /* Huffman table 10 - 64 entries */
    11, 11, 11, 11, 11, 11, 10, 10, 10, 10, 10, 10, 10, 11, 11, 10,  9,  9, 10,
    10,  9,  9, 10, 10,  9, 10, 10,  8,  8,  9,  9, 10, 10,  9,  9, 10, 10,  8,
     8,  8,  9,  9,  9,  9,  9,  9,  8,  8,  8,  8,  8,  8,  7,  7,  7,  7,  6,
     6,  6,  6,  4,  3,  3,  1,
    /* Huffman table 11 - 64 entries */
    10, 10, 10, 10, 10, 10, 10, 11, 11, 10, 10,  9,  9,  9, 10, 10, 10, 10,  8,
     8,  9,  9,  7,  8,  8,  8,  8,  8,  9,  9,  9,  9,  8,  7,  8,  8,  7,  7,
     8,  8,  8,  9,  9,  8,  8,  8,  8,  8,  8,  7,  7,  6,  6,  7,  7,  6,  5,
     4,  5,  5,  3,  3,  3,  2,
    /* Huffman table 12 - 64 entries */
    10, 10,  9,  9,  9,  9,  9,  9,  9,  8,  8,  9,  9,  8,  8,  8,  8,  8,  8,
     9,  9,  8,  8,  8,  8,  8,  9,  9,  7,  7,  7,  8,  8,  8,  8,  8,  8,  7,
     7,  7,  7,  8,  8,  7,  7,  7,  6,  6,  6,  6,  7,  7,  6,  5,  5,  5,  4,
     4,  5,  5,  4,  3,  3,  3,
    /* Huffman table 13 - 256 entries */
    19, 19, 18, 17, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 17, 17, 15, 15, 16,
    16, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 16, 16, 15, 16, 16, 14, 14, 15,
    15, 15, 15, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 15, 15, 14, 13, 14,
    14, 13, 13, 14, 14, 13, 14, 14, 13, 14, 14, 13, 14, 14, 13, 13, 14, 14, 12,
    12, 12, 13, 13, 13, 13, 13, 13, 12, 13, 13, 12, 12, 13, 13, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 12, 12, 13, 13, 12, 12, 12, 12, 13, 13, 13, 13, 12,
    13, 13, 12, 11, 12, 12, 12, 12, 12, 12, 12, 12, 11, 11, 11, 11, 12, 12, 11,
    11, 12, 12, 11, 12, 12, 12, 12, 11, 11, 12, 12, 11, 12, 12, 11, 12, 12, 11,
    12, 12, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 10, 10, 10, 10, 11, 11,
    10, 11, 11, 10, 11, 11, 11, 11, 10, 10, 11, 11, 10, 10, 11, 11, 11, 11, 11,
    11,  9,  9, 10, 10, 10, 10, 10, 11, 11,  9,  9,  9, 10, 10,  9,  9, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10,  8,  9,  9,  9,  9,  9,  9, 10, 10,  9,  9,
     9,  8,  8,  9,  9,  9,  9,  9,  9,  8,  7,  8,  8,  8,  8,  7,  7,  7,  7,
     7,  6,  6,  6,  6,  4,  4,  3,  1,
    /* Huffman table 15 - 256 entries */
    13, 13, 13, 13, 12, 13, 13, 13, 13, 13, 13, 12, 13, 13, 12, 12, 12, 12, 12,
    12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13,
    13, 11, 11, 12, 12, 12, 12, 11, 11, 11, 11, 11, 11, 12, 12, 11, 11, 11, 11,
    11, 11, 11, 11, 12, 12, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 12, 12, 11, 11, 11, 11, 11,
    11, 10, 11, 11, 11, 11, 11, 11, 10, 10, 11, 11, 10, 10, 10, 10, 11, 11, 10,
    10, 10, 10, 10, 10, 10, 11, 11, 10, 10, 10, 10, 10, 11, 11,  9, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10,  9, 10, 10, 10, 10,  9, 10, 10,  9, 10,
    10, 10, 10, 10, 10, 10, 10,  9,  9,  9,  9,  9,  9,  9, 10, 10,  9,  9,  9,
     9,  9,  9, 10, 10,  9,  9,  9,  9,  9,  9,  8,  9,  9,  9,  9,  9,  9,  9,
     9,  9,  9,  8,  8,  8,  8,  9,  9,  9,  9,  9,  9,  9,  9,  8,  8,  8,  8,
     8,  8,  9,  9,  8,  8,  8,  8,  8,  8,  8,  9,  9,  8,  7,  8,  8,  7,  7,
     7,  7,  8,  8,  7,  7,  7,  7,  7,  6,  7,  7,  6,  6,  7,  7,  6,  6,  6,
     5,  5,  5,  5,  5,  3,  4,  4,  3,
    /* Huffman table 16 - 256 entries */
    11, 11, 11, 11, 11, 11, 11, 11, 10, 11, 11, 11, 11, 10, 10, 10, 10, 10,  8,
    10, 10,  9,  9,  9,  9, 10, 16, 17, 17, 15, 15, 16, 16, 14, 15, 15, 14, 14,
    15, 15, 14, 14, 15, 15, 15, 15, 14, 15, 15, 14, 13,  8,  9,  9,  8,  8, 13,
    14, 14, 14, 14, 14, 14, 14, 14, 14, 14, 13, 13, 14, 14, 14, 14, 13, 14, 14,
    13, 13, 13, 14, 14, 14, 14, 13, 13, 14, 14, 13, 14, 14, 12, 13, 13, 13, 13,
    13, 13, 13, 13, 13, 13, 13, 13, 13, 13, 12, 13, 13, 13, 13, 13, 13, 12, 13,
    13, 12, 12, 13, 13, 11, 12, 12, 12, 12, 12, 12, 12, 13, 13, 11, 12, 12, 12,
    12, 11, 12, 12, 12, 12, 12, 12, 12, 12, 11, 12, 12, 11, 11, 11, 11, 12, 12,
    12, 12, 12, 12, 12, 12, 11, 12, 12, 11, 12, 12, 11, 12, 12, 11, 12, 12, 11,
    10, 10, 11, 11, 11, 11, 11, 11, 10, 10, 11, 11, 10, 10, 11, 11, 11, 11, 11,
    11, 11, 11, 10, 11, 11, 10, 10, 10, 11, 11, 10, 10, 11, 11, 10, 10, 11, 11,
    10,  9,  9, 10, 10, 10, 10, 10, 10,  9,  9,  9, 10, 10,  9, 10, 10,  9,  9,
     8,  9,  9,  9,  9,  9,  9,  9,  9,  8,  8,  9,  9,  8,  8,  7,  7,  8,  8,
     7,  6,  6,  6,  6,  4,  4,  3,  1,
    /* Huffman table 24 - 256 entries */
     8,  8,  8,  8,  8,  8,  8,  8,  7,  8,  8,  7,  7,  8,  8,  7,  7,  7,  7,
     7,  7,  7,  7,  7,  7,  7,  7,  8,  8,  9, 11, 11, 11, 11, 11, 11, 11, 11,
    11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11, 11,
    11,  4, 11, 11, 11, 11, 12, 12, 11, 10, 11, 11, 10, 10, 10, 10, 11, 11, 10,
    10, 10, 10, 11, 11, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 10, 10, 10, 10, 11, 11, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10,
    10, 10, 11, 11, 10, 11, 11, 10,  9, 10, 10, 10, 10, 11, 11, 10,  9,  9, 10,
    10,  9, 10, 10, 10, 10,  9,  9, 10, 10,  9,  9,  9,  9,  9,  9,  9,  9,  9,
     9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,  9,
     9,  9,  9,  9,  9,  9,  9,  9, 10, 10,  9,  9,  9, 10, 10,  8,  9,  9,  8,
     8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  9,  9,  8,  8,  8,  8,  8,
     8,  9,  9,  7,  8,  8,  7,  7,  7,  7,  7,  8,  8,  7,  7,  6,  6,  7,  7,
     6,  5,  5,  6,  6,  4,  4,  4,  4,
};

static const uint8_t mpa_huffsymbols[] = {
    /* Huffman table 1 - 4 entries */
    0x11, 0x01, 0x10, 0x00,
    /* Huffman table 2 - 9 entries */
    0x22, 0x02, 0x12, 0x21, 0x20, 0x11, 0x01, 0x10, 0x00,
    /* Huffman table 3 - 9 entries */
    0x22, 0x02, 0x12, 0x21, 0x20, 0x10, 0x11, 0x01, 0x00,
    /* Huffman table 5 - 16 entries */
    0x33, 0x23, 0x32, 0x31, 0x13, 0x03, 0x30, 0x22, 0x12, 0x21, 0x02, 0x20,
    0x11, 0x01, 0x10, 0x00,
    /* Huffman table 6 - 16 entries */
    0x33, 0x03, 0x23, 0x32, 0x30, 0x13, 0x31, 0x22, 0x02, 0x12, 0x21, 0x20,
    0x01, 0x11, 0x10, 0x00,
    /* Huffman table 7 - 36 entries */
    0x55, 0x45, 0x54, 0x53, 0x35, 0x44, 0x25, 0x52, 0x15, 0x51, 0x05, 0x34,
    0x50, 0x43, 0x33, 0x24, 0x42, 0x14, 0x41, 0x40, 0x04, 0x23, 0x32, 0x03,
    0x13, 0x31, 0x30, 0x22, 0x12, 0x21, 0x02, 0x20, 0x11, 0x01, 0x10, 0x00,
    /* Huffman table 8 - 36 entries */
    0x55, 0x54, 0x45, 0x53, 0x35, 0x44, 0x25, 0x52, 0x05, 0x15, 0x51, 0x34,
    0x43, 0x50, 0x33, 0x24, 0x42, 0x14, 0x41, 0x04, 0x40, 0x23, 0x32, 0x13,
    0x31, 0x03, 0x30, 0x22, 0x02, 0x20, 0x12, 0x21, 0x11, 0x01, 0x10, 0x00,
    /* Huffman table 9 - 36 entries */
    0x55, 0x45, 0x35, 0x53, 0x54, 0x05, 0x44, 0x25, 0x52, 0x15, 0x51, 0x34,
    0x43, 0x50, 0x04, 0x24, 0x42, 0x33, 0x40, 0x14, 0x41, 0x23, 0x32, 0x13,
    0x31, 0x03, 0x30, 0x22, 0x02, 0x12, 0x21, 0x20, 0x11, 0x01, 0x10, 0x00,
    /* Huffman table 10 - 64 entries */
    0x77, 0x67, 0x76, 0x57, 0x75, 0x66, 0x47, 0x74, 0x56, 0x65, 0x37, 0x73,
    0x46, 0x55, 0x54, 0x63, 0x27, 0x72, 0x64, 0x07, 0x70, 0x62, 0x45, 0x35,
    0x06, 0x53, 0x44, 0x17, 0x71, 0x36, 0x26, 0x25, 0x52, 0x15, 0x51, 0x34,
    0x43, 0x16, 0x61, 0x60, 0x05, 0x50, 0x24, 0x42, 0x33, 0x04, 0x14, 0x41,
    0x40, 0x23, 0x32, 0x03, 0x13, 0x31, 0x30, 0x22, 0x12, 0x21, 0x02, 0x20,
    0x11, 0x01, 0x10, 0x00,
    /* Huffman table 11 - 64 entries */
    0x77, 0x67, 0x76, 0x75, 0x66, 0x47, 0x74, 0x57, 0x55, 0x56, 0x65, 0x37,
    0x73, 0x46, 0x45, 0x54, 0x35, 0x53, 0x27, 0x72, 0x64, 0x07, 0x71, 0x17,
    0x70, 0x36, 0x63, 0x60, 0x44, 0x25, 0x52, 0x05, 0x15, 0x62, 0x26, 0x06,
    0x16, 0x61, 0x51, 0x34, 0x50, 0x43, 0x33, 0x24, 0x42, 0x14, 0x41, 0x04,
    0x40, 0x23, 0x32, 0x13, 0x31, 0x03, 0x30, 0x22, 0x21, 0x12, 0x02, 0x20,
    0x11, 0x01, 0x10, 0x00,
    /* Huffman table 12 - 64 entries */
    0x77, 0x67, 0x76, 0x57, 0x75, 0x66, 0x47, 0x74, 0x65, 0x56, 0x37, 0x73,
    0x55, 0x27, 0x72, 0x46, 0x64, 0x17, 0x71, 0x07, 0x70, 0x36, 0x63, 0x45,
    0x54, 0x44, 0x06, 0x05, 0x26, 0x62, 0x61, 0x16, 0x60, 0x35, 0x53, 0x25,
    0x52, 0x15, 0x51, 0x34, 0x43, 0x50, 0x04, 0x24, 0x42, 0x14, 0x33, 0x41,
    0x23, 0x32, 0x40, 0x03, 0x30, 0x13, 0x31, 0x22, 0x12, 0x21, 0x02, 0x20,
    0x00, 0x11, 0x01, 0x10,
    /* Huffman table 13 - 256 entries */
    0xFE, 0xFC, 0xFD, 0xED, 0xFF, 0xEF, 0xDF, 0xEE, 0xCF, 0xDE, 0xBF, 0xFB,
    0xCE, 0xDC, 0xAF, 0xE9, 0xEC, 0xDD, 0xFA, 0xCD, 0xBE, 0xEB, 0x9F, 0xF9,
    0xEA, 0xBD, 0xDB, 0x8F, 0xF8, 0xCC, 0xAE, 0x9E, 0x8E, 0x7F, 0x7E, 0xF7,
    0xDA, 0xAD, 0xBC, 0xCB, 0xF6, 0x6F, 0xE8, 0x5F, 0x9D, 0xD9, 0xF5, 0xE7,
    0xAC, 0xBB, 0x4F, 0xF4, 0xCA, 0xE6, 0xF3, 0x3F, 0x8D, 0xD8, 0x2F, 0xF2,
    0x6E, 0x9C, 0x0F, 0xC9, 0x5E, 0xAB, 0x7D, 0xD7, 0x4E, 0xC8, 0xD6, 0x3E,
    0xB9, 0x9B, 0xAA, 0x1F, 0xF1, 0xF0, 0xBA, 0xE5, 0xE4, 0x8C, 0x6D, 0xE3,
    0xE2, 0x2E, 0x0E, 0x1E, 0xE1, 0xE0, 0x5D, 0xD5, 0x7C, 0xC7, 0x4D, 0x8B,
    0xB8, 0xD4, 0x9A, 0xA9, 0x6C, 0xC6, 0x3D, 0xD3, 0x7B, 0x2D, 0xD2, 0x1D,
    0xB7, 0x5C, 0xC5, 0x99, 0x7A, 0xC3, 0xA7, 0x97, 0x4B, 0xD1, 0x0D, 0xD0,
    0x8A, 0xA8, 0x4C, 0xC4, 0x6B, 0xB6, 0x3C, 0x2C, 0xC2, 0x5B, 0xB5, 0x89,
    0x1C, 0xC1, 0x98, 0x0C, 0xC0, 0xB4, 0x6A, 0xA6, 0x79, 0x3B, 0xB3, 0x88,
    0x5A, 0x2B, 0xA5, 0x69, 0xA4, 0x78, 0x87, 0x94, 0x77, 0x76, 0xB2, 0x1B,
    0xB1, 0x0B, 0xB0, 0x96, 0x4A, 0x3A, 0xA3, 0x59, 0x95, 0x2A, 0xA2, 0x1A,
    0xA1, 0x0A, 0x68, 0xA0, 0x86, 0x49, 0x93, 0x39, 0x58, 0x85, 0x67, 0x29,
    0x92, 0x57, 0x75, 0x38, 0x83, 0x66, 0x47, 0x74, 0x56, 0x65, 0x73, 0x19,
    0x91, 0x09, 0x90, 0x48, 0x84, 0x72, 0x46, 0x64, 0x28, 0x82, 0x18, 0x37,
    0x27, 0x17, 0x71, 0x55, 0x07, 0x70, 0x36, 0x63, 0x45, 0x54, 0x26, 0x62,
    0x35, 0x81, 0x08, 0x80, 0x16, 0x61, 0x06, 0x60, 0x53, 0x44, 0x25, 0x52,
    0x05, 0x15, 0x51, 0x34, 0x43, 0x50, 0x24, 0x42, 0x33, 0x14, 0x41, 0x04,
    0x40, 0x23, 0x32, 0x13, 0x31, 0x03, 0x30, 0x22, 0x12, 0x21, 0x02, 0x20,
    0x11, 0x01, 0x10, 0x00,
    /* Huffman table 15 - 256 entries */
    0xFF, 0xEF, 0xFE, 0xDF, 0xEE, 0xFD, 0xCF, 0xFC, 0xDE, 0xED, 0xBF, 0xFB,
    0xCE, 0xEC, 0xDD, 0xAF, 0xFA, 0xBE, 0xEB, 0xCD, 0xDC, 0x9F, 0xF9, 0xEA,
    0xBD, 0xDB, 0x8F, 0xF8, 0xCC, 0x9E, 0xE9, 0x7F, 0xF7, 0xAD, 0xDA, 0xBC,
    0x6F, 0xAE, 0x0F, 0xCB, 0xF6, 0x8E, 0xE8, 0x5F, 0x9D, 0xF5, 0x7E, 0xE7,
    0xAC, 0xCA, 0xBB, 0xD9, 0x8D, 0x4F, 0xF4, 0x3F, 0xF3, 0xD8, 0xE6, 0x2F,
    0xF2, 0x6E, 0xF0, 0x1F, 0xF1, 0x9C, 0xC9, 0x5E, 0xAB, 0xBA, 0xE5, 0x7D,
    0xD7, 0x4E, 0xE4, 0x8C, 0xC8, 0x3E, 0x6D, 0xD6, 0xE3, 0x9B, 0xB9, 0x2E,
    0xAA, 0xE2, 0x1E, 0xE1, 0x0E, 0xE0, 0x5D, 0xD5, 0x7C, 0xC7, 0x4D, 0x8B,
    0xD4, 0xB8, 0x9A, 0xA9, 0x6C, 0xC6, 0x3D, 0xD3, 0xD2, 0x2D, 0x0D, 0x1D,
    0x7B, 0xB7, 0xD1, 0x5C, 0xD0, 0xC5, 0x8A, 0xA8, 0x4C, 0xC4, 0x6B, 0xB6,
    0x99, 0x0C, 0x3C, 0xC3, 0x7A, 0xA7, 0xA6, 0xC0, 0x0B, 0xC2, 0x2C, 0x5B,
    0xB5, 0x1C, 0x89, 0x98, 0xC1, 0x4B, 0xB4, 0x6A, 0x3B, 0x79, 0xB3, 0x97,
    0x88, 0x2B, 0x5A, 0xB2, 0xA5, 0x1B, 0xB1, 0xB0, 0x69, 0x96, 0x4A, 0xA4,
    0x78, 0x87, 0x3A, 0xA3, 0x59, 0x95, 0x2A, 0xA2, 0x1A, 0xA1, 0x0A, 0xA0,
    0x68, 0x86, 0x49, 0x94, 0x39, 0x93, 0x77, 0x09, 0x58, 0x85, 0x29, 0x67,
    0x76, 0x92, 0x91, 0x19, 0x90, 0x48, 0x84, 0x57, 0x75, 0x38, 0x83, 0x66,
    0x47, 0x28, 0x82, 0x18, 0x81, 0x74, 0x08, 0x80, 0x56, 0x65, 0x37, 0x73,
    0x46, 0x27, 0x72, 0x64, 0x17, 0x55, 0x71, 0x07, 0x70, 0x36, 0x63, 0x45,
    0x54, 0x26, 0x62, 0x16, 0x06, 0x60, 0x35, 0x61, 0x53, 0x44, 0x25, 0x52,
    0x15, 0x51, 0x05, 0x50, 0x34, 0x43, 0x24, 0x42, 0x33, 0x41, 0x14, 0x04,
    0x23, 0x32, 0x40, 0x03, 0x13, 0x31, 0x30, 0x22, 0x12, 0x21, 0x02, 0x20,
    0x11, 0x01, 0x10, 0x00,
    /* Huffman table 16 - 256 entries */
    0xEF, 0xFE, 0xDF, 0xFD, 0xCF, 0xFC, 0xBF, 0xFB, 0xAF, 0xFA, 0x9F, 0xF9,
    0xF8, 0x8F, 0x7F, 0xF7, 0x6F, 0xF6, 0xFF, 0x5F, 0xF5, 0x4F, 0xF4, 0xF3,
    0xF0, 0x3F, 0xCE, 0xEC, 0xDD, 0xDE, 0xE9, 0xEA, 0xD9, 0xEE, 0xED, 0xEB,
    0xBE, 0xCD, 0xDC, 0xDB, 0xAE, 0xCC, 0xAD, 0xDA, 0x7E, 0xAC, 0xCA, 0xC9,
    0x7D, 0x5E, 0xBD, 0xF2, 0x2F, 0x0F, 0x1F, 0xF1, 0x9E, 0xBC, 0xCB, 0x8E,
    0xE8, 0x9D, 0xE7, 0xBB, 0x8D, 0xD8, 0x6E, 0xE6, 0x9C, 0xAB, 0xBA, 0xE5,
    0xD7, 0x4E, 0xE4, 0x8C, 0xC8, 0x3E, 0x6D, 0xD6, 0x9B, 0xB9, 0xAA, 0xE1,
    0xD4, 0xB8, 0xA9, 0x7B, 0xB7, 0xD0, 0xE3, 0x0E, 0xE0, 0x5D, 0xD5, 0x7C,
    0xC7, 0x4D, 0x8B, 0x9A, 0x6C, 0xC6, 0x3D, 0x5C, 0xC5, 0x0D, 0x8A, 0xA8,
    0x99, 0x4C, 0xB6, 0x7A, 0x3C, 0x5B, 0x89, 0x1C, 0xC0, 0x98, 0x79, 0xE2,
    0x2E, 0x1E, 0xD3, 0x2D, 0xD2, 0xD1, 0x3B, 0x97, 0x88, 0x1D, 0xC4, 0x6B,
    0xC3, 0xA7, 0x2C, 0xC2, 0xB5, 0xC1, 0x0C, 0x4B, 0xB4, 0x6A, 0xA6, 0xB3,
    0x5A, 0xA5, 0x2B, 0xB2, 0x1B, 0xB1, 0x0B, 0xB0, 0x69, 0x96, 0x4A, 0xA4,
    0x78, 0x87, 0xA3, 0x3A, 0x59, 0x2A, 0x95, 0x68, 0xA1, 0x86, 0x77, 0x94,
    0x49, 0x57, 0x67, 0xA2, 0x1A, 0x0A, 0xA0, 0x39, 0x93, 0x58, 0x85, 0x29,
    0x92, 0x76, 0x09, 0x19, 0x91, 0x90, 0x48, 0x84, 0x75, 0x38, 0x83, 0x66,
    0x28, 0x82, 0x47, 0x74, 0x18, 0x81, 0x80, 0x08, 0x56, 0x37, 0x73, 0x65,
    0x46, 0x27, 0x72, 0x64, 0x55, 0x07, 0x17, 0x71, 0x70, 0x36, 0x63, 0x45,
    0x54, 0x26, 0x62, 0x16, 0x61, 0x06, 0x60, 0x53, 0x35, 0x44, 0x25, 0x52,
    0x51, 0x15, 0x05, 0x34, 0x43, 0x50, 0x24, 0x42, 0x33, 0x14, 0x41, 0x04,
    0x40, 0x23, 0x32, 0x13, 0x31, 0x03, 0x30, 0x22, 0x12, 0x21, 0x02, 0x20,
    0x11, 0x01, 0x10, 0x00,
    /* Huffman table 24 - 256 entries */
    0xEF, 0xFE, 0xDF, 0xFD, 0xCF, 0xFC, 0xBF, 0xFB, 0xFA, 0xAF, 0x9F, 0xF9,
    0xF8, 0x8F, 0x7F, 0xF7, 0x6F, 0xF6, 0x5F, 0xF5, 0x4F, 0xF4, 0x3F, 0xF3,
    0x2F, 0xF2, 0xF1, 0x1F, 0xF0, 0x0F, 0xEE, 0xDE, 0xED, 0xCE, 0xEC, 0xDD,
    0xBE, 0xEB, 0xCD, 0xDC, 0xAE, 0xEA, 0xBD, 0xDB, 0xCC, 0x9E, 0xE9, 0xAD,
    0xDA, 0xBC, 0xCB, 0x8E, 0xE8, 0x9D, 0xD9, 0x7E, 0xE7, 0xAC, 0xFF, 0xCA,
    0xBB, 0x8D, 0xD8, 0x0E, 0xE0, 0x0D, 0xE6, 0x6E, 0x9C, 0xC9, 0x5E, 0xBA,
    0xE5, 0xAB, 0x7D, 0xD7, 0xE4, 0x8C, 0xC8, 0x4E, 0x2E, 0x3E, 0x6D, 0xD6,
    0xE3, 0x9B, 0xB9, 0xAA, 0xE2, 0x1E, 0xE1, 0x5D, 0xD5, 0x7C, 0xC7, 0x4D,
    0x8B, 0xB8, 0xD4, 0x9A, 0xA9, 0x6C, 0xC6, 0x3D, 0xD3, 0x2D, 0xD2, 0x1D,
    0x7B, 0xB7, 0xD1, 0x5C, 0xC5, 0x8A, 0xA8, 0x99, 0x4C, 0xC4, 0x6B, 0xB6,
    0xD0, 0x0C, 0x3C, 0xC3, 0x7A, 0xA7, 0x2C, 0xC2, 0x5B, 0xB5, 0x1C, 0x89,
    0x98, 0xC1, 0x4B, 0xC0, 0x0B, 0x3B, 0xB0, 0x0A, 0x1A, 0xB4, 0x6A, 0xA6,
    0x79, 0x97, 0xA0, 0x09, 0x90, 0xB3, 0x88, 0x2B, 0x5A, 0xB2, 0xA5, 0x1B,
    0xB1, 0x69, 0x96, 0xA4, 0x4A, 0x78, 0x87, 0x3A, 0xA3, 0x59, 0x95, 0x2A,
    0xA2, 0xA1, 0x68, 0x86, 0x77, 0x49, 0x94, 0x39, 0x93, 0x58, 0x85, 0x29,
    0x67, 0x76, 0x92, 0x19, 0x91, 0x48, 0x84, 0x57, 0x75, 0x38, 0x83, 0x66,
    0x28, 0x82, 0x18, 0x47, 0x74, 0x81, 0x08, 0x80, 0x56, 0x65, 0x17, 0x07,
    0x70, 0x73, 0x37, 0x27, 0x72, 0x46, 0x64, 0x55, 0x71, 0x36, 0x63, 0x45,
    0x54, 0x26, 0x62, 0x16, 0x61, 0x06, 0x60, 0x35, 0x53, 0x44, 0x25, 0x52,
    0x15, 0x05, 0x50, 0x51, 0x34, 0x43, 0x24, 0x42, 0x33, 0x14, 0x41, 0x04,
    0x40, 0x23, 0x32, 0x13, 0x31, 0x03, 0x30, 0x22, 0x12, 0x21, 0x02, 0x20,
    0x11, 0x01, 0x10, 0x00,
};

static const uint8_t mpa_huff_sizes_minus_one[] =
{
    3, 8, 8, 15, 15, 35, 35, 35, 63, 63, 63, 255, 255, 255, 255
};

const uint8_t ff_mpa_huff_data[32][2] = {
{ 0, 0 },
{ 1, 0 },
{ 2, 0 },
{ 3, 0 },
{ 0, 0 },
{ 4, 0 },
{ 5, 0 },
{ 6, 0 },
{ 7, 0 },
{ 8, 0 },
{ 9, 0 },
{ 10, 0 },
{ 11, 0 },
{ 12, 0 },
{ 0, 0 },
{ 13, 0 },
{ 14, 1 },
{ 14, 2 },
{ 14, 3 },
{ 14, 4 },
{ 14, 6 },
{ 14, 8 },
{ 14, 10 },
{ 14, 13 },
{ 15, 4 },
{ 15, 5 },
{ 15, 6 },
{ 15, 7 },
{ 15, 8 },
{ 15, 9 },
{ 15, 11 },
{ 15, 13 },
};


/* huffman tables for quadrules */
static const uint8_t mpa_quad_codes[2][16] = {
    {  1,  5,  4,  5,  6,  5,  4,  4, 7,  3,  6,  0,  7,  2,  3,  1, },
    { 15, 14, 13, 12, 11, 10,  9,  8, 7,  6,  5,  4,  3,  2,  1,  0, },
};

static const uint8_t mpa_quad_bits[2][16] = {
    { 1, 4, 4, 5, 4, 6, 5, 6, 4, 5, 5, 6, 5, 6, 6, 6, },
    { 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, },
};

const uint8_t ff_band_size_long[9][22] = {
{ 4, 4, 4, 4, 4, 4, 6, 6, 8, 8, 10,
  12, 16, 20, 24, 28, 34, 42, 50, 54, 76, 158, }, /* 44100 */
{ 4, 4, 4, 4, 4, 4, 6, 6, 6, 8, 10,
  12, 16, 18, 22, 28, 34, 40, 46, 54, 54, 192, }, /* 48000 */
{ 4, 4, 4, 4, 4, 4, 6, 6, 8, 10, 12,
  16, 20, 24, 30, 38, 46, 56, 68, 84, 102, 26, }, /* 32000 */
{ 6, 6, 6, 6, 6, 6, 8, 10, 12, 14, 16,
  20, 24, 28, 32, 38, 46, 52, 60, 68, 58, 54, }, /* 22050 */
{ 6, 6, 6, 6, 6, 6, 8, 10, 12, 14, 16,
  18, 22, 26, 32, 38, 46, 52, 64, 70, 76, 36, }, /* 24000 */
{ 6, 6, 6, 6, 6, 6, 8, 10, 12, 14, 16,
  20, 24, 28, 32, 38, 46, 52, 60, 68, 58, 54, }, /* 16000 */
{ 6, 6, 6, 6, 6, 6, 8, 10, 12, 14, 16,
  20, 24, 28, 32, 38, 46, 52, 60, 68, 58, 54, }, /* 11025 */
{ 6, 6, 6, 6, 6, 6, 8, 10, 12, 14, 16,
  20, 24, 28, 32, 38, 46, 52, 60, 68, 58, 54, }, /* 12000 */
{ 12, 12, 12, 12, 12, 12, 16, 20, 24, 28, 32,
  40, 48, 56, 64, 76, 90, 2, 2, 2, 2, 2, }, /* 8000 */
};

const uint8_t ff_band_size_short[9][13] = {
{ 4, 4, 4, 4, 6, 8, 10, 12, 14, 18, 22, 30, 56, }, /* 44100 */
{ 4, 4, 4, 4, 6, 6, 10, 12, 14, 16, 20, 26, 66, }, /* 48000 */
{ 4, 4, 4, 4, 6, 8, 12, 16, 20, 26, 34, 42, 12, }, /* 32000 */
{ 4, 4, 4, 6, 6, 8, 10, 14, 18, 26, 32, 42, 18, }, /* 22050 */
{ 4, 4, 4, 6, 8, 10, 12, 14, 18, 24, 32, 44, 12, }, /* 24000 */
{ 4, 4, 4, 6, 8, 10, 12, 14, 18, 24, 30, 40, 18, }, /* 16000 */
{ 4, 4, 4, 6, 8, 10, 12, 14, 18, 24, 30, 40, 18, }, /* 11025 */
{ 4, 4, 4, 6, 8, 10, 12, 14, 18, 24, 30, 40, 18, }, /* 12000 */
{ 8, 8, 8, 12, 16, 20, 24, 28, 36, 2, 2, 2, 26, }, /* 8000 */
};

uint16_t ff_band_index_long[9][23];

const uint8_t ff_mpa_pretab[2][22] = {
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 3, 3, 3, 2, 0 },
};

static av_cold void mpegaudiodec_common_init_static(void)
{
    const uint8_t *huff_sym = mpa_huffsymbols, *huff_lens = mpa_hufflens;
    int offset;

    /* scale factors table for layer 1/2 */
    for (int i = 0; i < 64; i++) {
        int shift, mod;
        /* 1.0 (i = 3) is normalized to 2 ^ FRAC_BITS */
        shift = i / 3;
        mod   = i % 3;
        ff_scale_factor_modshift[i] = mod | (shift << 2);
    }

    /* huffman decode tables */
    offset = 0;
    for (int i = 0; i < 15;) {
        uint16_t tmp_symbols[256];
        int nb_codes_minus_one = mpa_huff_sizes_minus_one[i];
        int j;

        for (j = 0; j <= nb_codes_minus_one; j++) {
            uint8_t high = huff_sym[j] & 0xF0, low = huff_sym[j] & 0xF;

            tmp_symbols[j] = high << 1 | ((high && low) << 4) | low;
        }

        ff_huff_vlc[++i].table         = huff_vlc_tables + offset;
        ff_huff_vlc[i].table_allocated = FF_ARRAY_ELEMS(huff_vlc_tables) - offset;
        ff_init_vlc_from_lengths(&ff_huff_vlc[i], 7, j,
                                 huff_lens, 1, tmp_symbols, 2, 2,
                                 0, INIT_VLC_STATIC_OVERLONG, NULL);
        offset    += ff_huff_vlc[i].table_size;
        huff_lens += j;
        huff_sym  += j;
    }
    av_assert0(offset == FF_ARRAY_ELEMS(huff_vlc_tables));

    offset = 0;
    for (int i = 0; i < 2; i++) {
        int bits = i == 0 ? 6 : 4;
        ff_huff_quad_vlc[i].table = huff_quad_vlc_tables + offset;
        ff_huff_quad_vlc[i].table_allocated = 1 << bits;
        offset                             += 1 << bits;
        init_vlc(&ff_huff_quad_vlc[i], bits, 16,
                 mpa_quad_bits[i], 1, 1, mpa_quad_codes[i], 1, 1,
                 INIT_VLC_USE_NEW_STATIC);
    }
    av_assert0(offset == FF_ARRAY_ELEMS(huff_quad_vlc_tables));

    for (int i = 0; i < 9; i++) {
        int k = 0;
        for (int j = 0; j < 22; j++) {
            ff_band_index_long[i][j] = k;
            k += ff_band_size_long[i][j] >> 1;
        }
        ff_band_index_long[i][22] = k;
    }

    for (int i = 0; i < 4; i++) {
        if (ff_mpa_quant_bits[i] < 0) {
            for (int j = 0; j < (1 << (-ff_mpa_quant_bits[i] + 1)); j++) {
                int val1, val2, val3, steps;
                int val = j;
                steps   = ff_mpa_quant_steps[i];
                val1    = val % steps;
                val    /= steps;
                val2    = val % steps;
                val3    = val / steps;
                ff_division_tabs[i][j] = val1 + (val2 << 4) + (val3 << 8);
            }
        }
    }
    mpegaudiodec_common_tableinit();
}

av_cold void ff_mpegaudiodec_common_init_static(void)
{
    static AVOnce init_static_once = AV_ONCE_INIT;

    ff_thread_once(&init_static_once, mpegaudiodec_common_init_static);
}
