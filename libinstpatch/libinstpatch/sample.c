/*
 * libInstPatch
 * Copyright (C) 1999-2014 Element Green <element@elementsofsound.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; version 2.1
 * of the License only.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA or on the web at http://www.gnu.org.
 */
/**
 * SECTION: sample
 * @short_description: Audio sample format conversion functions and defines.
 * @see_also: 
 * @stability: Stable
 *
 * This module provides functions for converting audio formats as well as a system
 * for defining audio formats with a single integer composed of multiple fields
 * for sample width, channel count, sign and endian byte order.
 */
#include <glib.h>
#include <glib-object.h>
#include "sample.h"
#include "IpatchSampleTransform.h"

/* NOTES:
 * 24 bit is converted to 4 byte integers first (less of a pain)
 * Floating point audio is assumed to have a range of -1.0 to 1.0.
 */


#define NOP  (void)0;

#define TRANS_FUNC(NAME, INTYPE, OUTTYPE, PRE, CODE, POST) \
void TFF_ ## NAME (IpatchSampleTransform *transform) \
{ \
  INTYPE *inp = transform->buf1; \
  OUTTYPE *outp = transform->buf2; \
  guint i, count = transform->samples; \
  PRE \
 \
  for (i = 0; i < count; i++) \
  CODE; \
  POST \
}

/* float transforms */

TRANS_FUNC (floattodouble, gfloat, gdouble, NOP, { outp[i] = inp[i]; }, NOP)
TRANS_FUNC (doubletofloat, gdouble, gfloat, NOP, { outp[i] = inp[i]; }, NOP)


/* signed bit width change funcs */

TRANS_FUNC (s8to16, gint8, gint16, NOP, { outp[i] = (gint16)inp[i] << 8; }, NOP)
TRANS_FUNC (s8to24, gint8, gint32, NOP, { outp[i] = (gint32)inp[i] << 16; }, NOP)
TRANS_FUNC (s8to32, gint8, gint32, NOP, { outp[i] = (gint32)inp[i] << 24; }, NOP)
TRANS_FUNC (s8tofloat, gint8, gfloat, NOP,
            { outp[i] = inp[i] / (float)128.0; }, NOP)
TRANS_FUNC (s8todouble, gint8, gdouble, NOP,
            { outp[i] = inp[i] / (double)128.0; }, NOP)

TRANS_FUNC (s16to8, gint16, gint8, NOP, { outp[i] = inp[i] >> 8; }, NOP)
TRANS_FUNC (s16to24, gint16, gint32, NOP, { outp[i] = inp[i] << 8; }, NOP)
TRANS_FUNC (s16to32, gint16, gint32, NOP, { outp[i] = inp[i] << 16; }, NOP)
TRANS_FUNC (s16tofloat, gint16, gfloat, NOP,
            { outp[i] = inp[i] / (float)32768.0; }, NOP)
TRANS_FUNC (s16todouble, gint16, gdouble, NOP,
            { outp[i] = inp[i] / (double)32768.0; }, NOP)

TRANS_FUNC (s24to8, gint32, gint8, NOP, { outp[i] = inp[i] >> 16; }, NOP)
TRANS_FUNC (s24to16, gint32, gint16, NOP, { outp[i] = inp[i] >> 8; }, NOP)
TRANS_FUNC (s24to32, gint32, gint32, NOP, { outp[i] = inp[i] << 8; }, NOP)
TRANS_FUNC (s24tofloat, gint32, gfloat, NOP,
            { outp[i] = inp[i] / (float)8388608.0; }, NOP)
TRANS_FUNC (s24todouble, gint32, gdouble, NOP,
            { outp[i] = inp[i] / (double)8388608.0; }, NOP)

TRANS_FUNC (s32to8, gint32, gint8, NOP, { outp[i] = inp[i] >> 24; }, NOP)
TRANS_FUNC (s32to16, gint32, gint16, NOP, { outp[i] = inp[i] >> 16; }, NOP)
TRANS_FUNC (s32to24, gint32, gint32, NOP, { outp[i] = inp[i] >> 8; }, NOP)
TRANS_FUNC (s32tofloat, gint32, gfloat, NOP,
            { outp[i] = inp[i] / (float)2147483648.0; }, NOP)
TRANS_FUNC (s32todouble, gint32, gdouble, NOP,
            { outp[i] = inp[i] / (double)2147483648.0; }, NOP)

TRANS_FUNC (floattos8, gfloat, gint8, NOP,
            { outp[i] = inp[i] * 127.0; }, NOP)
TRANS_FUNC (floattos16, gfloat, gint16, NOP,
            { outp[i] = inp[i] * 32767.0; }, NOP)
TRANS_FUNC (floattos24, gfloat, gint32, NOP,
            { outp[i] = inp[i] * 8388607.0; }, NOP)
TRANS_FUNC (floattos32, gfloat, gint32, NOP,
            { outp[i] = inp[i] * 2147483647.0; }, NOP)

TRANS_FUNC (doubletos8, gdouble, gint8, NOP,
            { outp[i] = inp[i] * 127.0; }, NOP)
TRANS_FUNC (doubletos16, gdouble, gint16, NOP,
            { outp[i] = inp[i] * 32767.0; }, NOP)
TRANS_FUNC (doubletos24, gdouble, gint32, NOP,
            { outp[i] = inp[i] * 8388607.0; }, NOP)
TRANS_FUNC (doubletos32, gdouble, gint32, NOP,
            { outp[i] = inp[i] * 2147483647.0; }, NOP)


/* unsigned bit width change funcs */

TRANS_FUNC (u8to16, guint8, guint16, NOP, { outp[i] = inp[i] << 8; }, NOP)
TRANS_FUNC (u8to24, guint8, guint32, NOP, { outp[i] = inp[i] << 16; }, NOP)
TRANS_FUNC (u8to32, guint8, guint32, NOP, { outp[i] = inp[i] << 24; }, NOP)
TRANS_FUNC (u8tofloat, guint8, gfloat, NOP,
            { outp[i] = (gint8)(inp[i] ^ 0x80) / (float)128.0; }, NOP)
TRANS_FUNC (u8todouble, guint8, gdouble, NOP,
            { outp[i] = (gint8)(inp[i] ^ 0x80) / (double)128.0; }, NOP)

TRANS_FUNC (u16to8, guint16, guint8, NOP, { outp[i] = inp[i] >> 8; }, NOP)
TRANS_FUNC (u16to24, guint16, guint32, NOP, { outp[i] = inp[i] << 8; }, NOP)
TRANS_FUNC (u16to32, guint16, guint32, NOP, { outp[i] = inp[i] << 16; }, NOP)
TRANS_FUNC (u16tofloat, guint16, gfloat, NOP,
            { outp[i] = (gint16)(inp[i] ^ 0x8000) / (float)32768.0; }, NOP)
TRANS_FUNC (u16todouble, guint16, gdouble, NOP,
            { outp[i] = (gint16)(inp[i] ^ 0x8000) / (double)32768.0; }, NOP)

TRANS_FUNC (u24to8, guint32, guint8, NOP, { outp[i] = inp[i] >> 16; }, NOP)
TRANS_FUNC (u24to16, guint32, guint16, NOP, { outp[i] = inp[i] >> 8; }, NOP)
TRANS_FUNC (u24to32, guint32, guint32, NOP, { outp[i] = inp[i] << 8; }, NOP)
TRANS_FUNC (u24tofloat, guint32, gfloat, NOP,
            { outp[i] = ((gint32)inp[i] - 0x800000) / (float)8388608.0; }, NOP)
TRANS_FUNC (u24todouble, guint32, gdouble, NOP,
            { outp[i] = ((gint32)inp[i] - 0x800000) / (double)8388608.0; }, NOP)

TRANS_FUNC (u32to8, guint32, guint8, NOP, { outp[i] = inp[i] >> 24; }, NOP)
TRANS_FUNC (u32to16, guint32, guint16, NOP, { outp[i] = inp[i] >> 16; }, NOP)
TRANS_FUNC (u32to24, guint32, guint32, NOP, { outp[i] = inp[i] >> 8; }, NOP)
TRANS_FUNC (u32tofloat, guint32, gfloat, NOP,
            { outp[i] = (gint32)(inp[i] ^ 0x80000000) / (float)2147483648.0; }, NOP)
TRANS_FUNC (u32todouble, guint32, gdouble, NOP,
            { outp[i] = (gint32)(inp[i] ^ 0x80000000) / (double)2147483648.0; }, NOP)

TRANS_FUNC (floattou8, gfloat, guint8, NOP,
            { outp[i] = (inp[i] + 1.0) * 127.5 + 0.5; }, NOP)
TRANS_FUNC (floattou16, gfloat, guint16, NOP,
            { outp[i] = (inp[i] + 1.0) * 32767.5 + 0.5; }, NOP)
TRANS_FUNC (floattou24, gfloat, guint32, NOP,
            { outp[i] = (inp[i] + 1.0) * 8388607.5 + 0.5; }, NOP)
TRANS_FUNC (floattou32, gfloat, guint32, NOP,
            { outp[i] = (inp[i] + 1.0) * 2147483647.5 + 0.5; }, NOP)

TRANS_FUNC (doubletou8, gdouble, guint8, NOP,
            { outp[i] = (inp[i] + 1.0) * 127.5 + 0.5; }, NOP)
TRANS_FUNC (doubletou16, gdouble, guint16, NOP,
            { outp[i] = (inp[i] + 1.0) * 32767.5 + 0.5; }, NOP)
TRANS_FUNC (doubletou24, gdouble, guint32, NOP,
            { outp[i] = (inp[i] + 1.0) * 8388607.5 + 0.5; }, NOP)
TRANS_FUNC (doubletou32, gdouble, guint32, NOP,
            { outp[i] = (inp[i] + 1.0) * 2147483647.5 + 0.5; }, NOP)


/* sign changer funcs (24 bit in 4 byte integers requires 2 separate funcs) */

TRANS_FUNC (togsign8, guint8, guint8, NOP, { outp[i] = inp[i] ^ 0x80; }, NOP)
TRANS_FUNC (togsign16, guint16, guint16, NOP,
            { outp[i] = inp[i] ^ 0x8000; }, NOP)
TRANS_FUNC (signtou24, guint32, guint32, NOP,
            { outp[i] = inp[i] + 0x800000; }, NOP)
TRANS_FUNC (unsigntos24, gint32, gint32, NOP,
            { outp[i] = ((inp[i] ^ 0x800000) << 8) >> 8; }, NOP)
TRANS_FUNC (togsign32, guint32, guint32, NOP,
            { outp[i] = inp[i] ^ 0x80000000; }, NOP)


/* endian swapping funcs */

TRANS_FUNC (swap16, guint16, guint16, guint16 t;,
            { t = inp[i]; outp[i] = t << 8 | t >> 8; }, NOP)
TRANS_FUNC (swap32, guint32, guint32, guint32 t;,
            { t = inp[i]; outp[i] = ((t & 0xFF) << 24) | ((t & 0xFF00) << 8)
	      | ((t & 0xFF0000) >> 8) | ((t & 0xFF000000) >> 24); }, NOP)
TRANS_FUNC (swap64, guint64, guint64, guint64 t;,
            { t = inp[i]; outp[i] = ((t & 0xFF) << 56) | ((t & 0xFF00) << 40)
	      | ((t & 0xFF0000) << 24) | ((t & 0xFF000000) << 8)
	      | ((t & G_GINT64_CONSTANT (0xFF00000000U)) >> 8)
	      | ((t & G_GINT64_CONSTANT (0xFF0000000000U)) >> 24)
	      | ((t & G_GINT64_CONSTANT (0xFF000000000000U)) >> 40)
	      | ((t & G_GINT64_CONSTANT (0xFF00000000000000U)) >> 56); }, NOP)


/* funcs for converting between real signed 24 bit (3 byte) and 4 byte words */

/* signed little endian 3 bytes to 4 bytes */
TRANS_FUNC (sle3bto4b, guint8, guint32, guint i2 = 0;,
	    { outp[i] = inp[i2] | (inp[i2 + 1] << 8) | (inp[i2 + 2] << 16)
              | ((inp[i2 + 2] & 0x80) ? 0xFF000000 : 0); i2 += 3; }, NOP)

/* signed big endian 3 bytes to 4 bytes */
TRANS_FUNC (sbe3bto4b, guint8, guint32, guint i2 = 0;,
	    { outp[i] = inp[i2 + 2] | (inp[i2 + 1] << 8) | (inp[i2] << 16)
              | ((inp[i2] & 0x80) ? 0xFF000000 : 0); i2 += 3; }, NOP)

/* 4 bytes to signed little endian 3 bytes */
TRANS_FUNC (4btosle3b, guint32, guint8, guint i2 = 0; gint32 t;,
	    { t = inp[i]; outp[i2] = t; outp[i2 + 1] = t >> 8;
	      outp[i2 + 2] = t >> 16; i2 += 3; }, NOP)

/* 4 bytes to signed big endian 3 bytes */
TRANS_FUNC (4btosbe3b, guint32, guint8, guint i2 = 0; gint32 t;,
	    { t = inp[i]; outp[i2 + 2] = t; outp[i2 + 1] = t >> 8;
	      outp[i2] = t >> 16; i2 += 3; }, NOP)

/* funcs for converting between real unsigned 24 bit (3 byte) and 4 byte words */

/* unsigned little endian 3 bytes to 4 bytes */
TRANS_FUNC (ule3bto4b, guint8, guint32, guint i2 = 0;,
	    { outp[i] = inp[i2] | (inp[i2 + 1] << 8) | (inp[i2 + 2] << 16);
              i2 += 3; }, NOP)

/* unsigned big endian 3 bytes to 4 bytes */
TRANS_FUNC (ube3bto4b, guint8, guint32, guint i2 = 0;,
	    { outp[i] = inp[i2 + 2] | (inp[i2 + 1] << 8) | (inp[i2] << 16);
              i2 += 3; }, NOP)

/* 4 bytes to unsigned little endian 3 bytes */
TRANS_FUNC (4btoule3b, guint32, guint8, guint i2 = 0; guint32 t;,
	    { t = inp[i]; outp[i2] = t; outp[i2 + 1] = t >> 8;
	      outp[i2 + 2] = t >> 16; i2 += 3; }, NOP)

/* 4 bytes to unsigned big endian 3 bytes */
TRANS_FUNC (4btoube3b, guint32, guint8, guint i2 = 0; guint32 t;,
	    { t = inp[i]; outp[i2 + 2] = t; outp[i2 + 1] = t >> 8;
	      outp[i2] = t >> 16; i2 += 3; }, NOP)

/* mono to stereo transforms */
TRANS_FUNC (8mtos, guint8, guint8, NOP,
	    { outp[i << 1] = inp[i]; outp[(i << 1) + 1] = inp[i]; },
	    transform->samples = count << 1;)
TRANS_FUNC (16mtos, guint16, guint16, NOP,
	    { outp[i << 1] = inp[i]; outp[(i << 1) + 1] = inp[i]; },
	    transform->samples = count << 1;)
TRANS_FUNC (32mtos, guint32, guint32, NOP,
	    { outp[i << 1] = inp[i]; outp[(i << 1) + 1] = inp[i]; },
	    transform->samples = count << 1;)
TRANS_FUNC (64mtos, guint64, guint64, NOP,
	    { outp[i << 1] = inp[i]; outp[(i << 1) + 1] = inp[i]; },
	    transform->samples = count << 1;)

/* stereo to left transforms */
TRANS_FUNC (8stol, guint8, guint8, count >>= 1;,
	    { outp[i] = inp[i << 1]; }, transform->samples = count;)
TRANS_FUNC (16stol, guint16, guint16, count >>= 1;,
	    { outp[i] = inp[i << 1]; }, transform->samples = count;)
TRANS_FUNC (32stol, guint32, guint32, count >>= 1;,
	    { outp[i] = inp[i << 1]; }, transform->samples = count;)
TRANS_FUNC (64stol, guint64, guint64, count >>= 1;,
	    { outp[i] = inp[i << 1]; }, transform->samples = count;)

/* stereo to right transforms */
TRANS_FUNC (8stor, guint8, guint8, count >>= 1;,
	    { outp[i] = inp[(i << 1) + 1]; }, transform->samples = count;)
TRANS_FUNC (16stor, guint16, guint16, count >>= 1;,
	    { outp[i] = inp[(i << 1) + 1]; }, transform->samples = count;)
TRANS_FUNC (32stor, guint32, guint32, count >>= 1;,
	    { outp[i] = inp[(i << 1) + 1]; }, transform->samples = count;)
TRANS_FUNC (64stor, guint64, guint64, count >>= 1;,
	    { outp[i] = inp[(i << 1) + 1]; }, transform->samples = count;)

/* arbitrary channel mapping */
TRANS_FUNC (8chanmap, guint8, guint8,
            int schans = IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT (transform->src_format); \
            int dchans = IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT (transform->dest_format); \
	    int di; int spos = 0; int dpos = 0;,
	    { \
	      for (di = 0; di < dchans; di++, dpos++) \
		outp[dpos] = inp[spos + transform->channel_map[di]]; \
	      spos += schans; \
	    },
	    NOP)
TRANS_FUNC (16chanmap, guint16, guint16,
            int schans = IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT (transform->src_format); \
            int dchans = IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT (transform->dest_format); \
	    int di; int spos = 0; int dpos = 0;,
	    { \
	      for (di = 0; di < dchans; di++, dpos++) \
		outp[dpos] = inp[spos + transform->channel_map[di]]; \
	      spos += schans; \
	    },
	    NOP)
TRANS_FUNC (32chanmap, guint32, guint32,
            int schans = IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT (transform->src_format); \
            int dchans = IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT (transform->dest_format); \
	    int di; int spos = 0; int dpos = 0;,
	    { \
	      for (di = 0; di < dchans; di++, dpos++) \
		outp[dpos] = inp[spos + transform->channel_map[di]]; \
	      spos += schans; \
	    },
	    NOP)
TRANS_FUNC (64chanmap, guint64, guint64,
            int schans = IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT (transform->src_format); \
            int dchans = IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT (transform->dest_format); \
	    int di; int spos = 0; int dpos = 0;,
	    { \
	      for (di = 0; di < dchans; di++, dpos++) \
		outp[dpos] = inp[spos + transform->channel_map[di]]; \
	      spos += schans; \
	    },
	    NOP)
            

/* signed transform func matrix [infmt][outfmt] */
static IpatchSampleTransformFunc signed_tff[6][6] = {
  { NULL, TFF_s8to16, TFF_s8to24, TFF_s8to32, TFF_s8tofloat, TFF_s8todouble },
  { TFF_s16to8, NULL, TFF_s16to24, TFF_s16to32, TFF_s16tofloat,
    TFF_s16todouble },
  { TFF_s24to8, TFF_s24to16, NULL, TFF_s24to32, TFF_s24tofloat,
    TFF_s24todouble },
  { TFF_s32to8, TFF_s32to16, TFF_s32to24, NULL, TFF_s32tofloat,
    TFF_s32todouble },
  { TFF_floattos8, TFF_floattos16, TFF_floattos24, TFF_floattos32, NULL,
    TFF_floattodouble },
  { TFF_doubletos8, TFF_doubletos16, TFF_doubletos24, TFF_doubletos32,
    TFF_doubletofloat, NULL }
};

/* unsiged transform func matrix [infmt][outfmt] */
static IpatchSampleTransformFunc unsigned_tff[6][6] = {
  { NULL, TFF_u8to16, TFF_u8to24, TFF_u8to32, TFF_u8tofloat, TFF_u8todouble },
  { TFF_u16to8, NULL, TFF_u16to24, TFF_u16to32, TFF_u16tofloat,
    TFF_u16todouble },
  { TFF_u24to8, TFF_u24to16, NULL, TFF_u24to32, TFF_u24tofloat,
    TFF_u24todouble },
  { TFF_u32to8, TFF_u32to16, TFF_u32to24, NULL, TFF_u32tofloat,
    TFF_u32todouble },
  { TFF_floattou8, TFF_floattou16, TFF_floattou24, TFF_floattou32, NULL,
    TFF_floattodouble },
  { TFF_doubletou8, TFF_doubletou16, TFF_doubletou24, TFF_doubletou32,
    TFF_doubletofloat, NULL }
};

/* sign toggle transform functions */
static IpatchSampleTransformFunc sign_tff[6] = {
  TFF_togsign8, TFF_togsign16, NULL, TFF_togsign32, NULL, NULL
};

/* endian swap functions */
static IpatchSampleTransformFunc swap_tff[6] = {
  NULL, TFF_swap16, TFF_swap32, TFF_swap32, TFF_swap32, TFF_swap64
};

/* mono to stereo transform functions */
static IpatchSampleTransformFunc mono_to_stereo_tff[6] = {
  TFF_8mtos, TFF_16mtos, TFF_32mtos, TFF_32mtos, TFF_32mtos, TFF_64mtos
};

/* stereo to left transform functions */
static IpatchSampleTransformFunc stereo_to_left_tff[6] = {
  TFF_8stol, TFF_16stol, TFF_32stol, TFF_32stol, TFF_32stol, TFF_64stol
};

/* stereo to right transform functions */
static IpatchSampleTransformFunc stereo_to_right_tff[6] = {
  TFF_8stor, TFF_16stor, TFF_32stor, TFF_32stor, TFF_32stor, TFF_64stor
};

/* arbitrary transform functions */
static IpatchSampleTransformFunc chanmap_tff[6] = {
  TFF_8chanmap, TFF_16chanmap, TFF_32chanmap, TFF_32chanmap, TFF_32chanmap, TFF_64chanmap
};

/* IpatchSampleWidth format sizes in bytes (last 8 reserved) */
guint ipatch_sample_width_sizes[16] = { 0, 1, 2, 4, 4, 4, 8, 3,
					0, 0, 0, 0, 0, 0, 0, 0 };


static inline void update_max_size (int *curmax, int format);

/**
 * ipatch_sample_format_bit_width:
 * @format: Sample format
 *
 * Like ipatch_sample_format_width() but gets the effective bit width of the
 * format.  Of note is this is not always equivelant to the format width * 8.
 * For example: #IPATCH_SAMPLE_FLOAT has an effective bit width of 23,
 * #IPATCH_SAMPLE_24BIT has an effective bit width of 24 but is stored in 32
 * bits.  This function is really only useful for comparing the relative
 * "quality" of formats, and the actual returned values may change in the
 * future.
 *
 * Returns: Effective bit width of format.
 */
int
ipatch_sample_format_bit_width (int format)
{
  int width;

  width = IPATCH_SAMPLE_FORMAT_GET_WIDTH (format);

  switch (width)
    {
    case IPATCH_SAMPLE_FLOAT:
      /* actually 24 with the sign bit, but we set it to 23 to be less than
         24 bit integer audio */
      return (23);
    case IPATCH_SAMPLE_DOUBLE:
      return (52);
    case IPATCH_SAMPLE_REAL24BIT:
      return (24);
      break;
    default:
      return (width * 8);
    }
}

/**
 * ipatch_sample_format_verify:
 * @format: Sample format (#IpatchSampleWidth | #IpatchSampleSign
 *   | #IpatchSampleEndian | #IpatchSampleChannel).
 * 
 * Verify a sample format integer.
 * 
 * Returns: %TRUE if valid, %FALSE otherwise
 */
gboolean
ipatch_sample_format_verify (int format)
{
  int width;

  width = format & IPATCH_SAMPLE_WIDTH_MASK;

  if (width < IPATCH_SAMPLE_8BIT || width > IPATCH_SAMPLE_REAL24BIT)
    return (FALSE);

  if (IPATCH_SAMPLE_FORMAT_IS_UNSIGNED (format)
      && (width == IPATCH_SAMPLE_FLOAT
	  || width == IPATCH_SAMPLE_DOUBLE)) return (FALSE);

  if (IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT (format) >= IPATCH_SAMPLE_MAX_CHANNELS)
    return (FALSE);

  return (TRUE);
}

/**
 * ipatch_sample_format_transform_verify:
 * @src_format: Source sample format
 * @dest_format: Destination sample format
 * @channel_map: Channel mapping (use #IPATCH_SAMPLE_UNITY_CHANNEL_MAP
 *   to map all input channels to the same output channels, see
 *   #IPATCH_SAMPLE_MAP_CHANNEL macro for constructing channel map values)
 *
 * Verify source and destination sample formats and channel map for a sample
 * transform operation.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 */
gboolean
ipatch_sample_format_transform_verify (int src_format, int dest_format,
                                       guint32 channel_map)
{
  int src_chans, dest_chans, i;

  if (!ipatch_sample_format_verify (src_format)
      || !ipatch_sample_format_verify (dest_format))
    return (FALSE);

  src_chans = IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT (src_format);
  dest_chans = IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT (dest_format);

  for (i = 0; i < dest_chans; i++)
    if (((channel_map >> (i * 3)) & 0x07) >= src_chans)
      return (FALSE);

  return (TRUE);
}

/**
 * ipatch_sample_get_transform_funcs: (skip)
 * @src_format: Source audio format to convert from
 * @dest_format: Destination audio format to convert to
 * @channel_map: Channel mapping (use #IPATCH_SAMPLE_UNITY_CHANNEL_MAP
 *   to map all input channels to the same output channels, 3 bits times
 *   #IPATCH_SAMPLE_MAX_CHANNELS (8) = 24 bits total, see
 *   #IPATCH_SAMPLE_MAP_CHANNEL macro for constructing channel map values)
 * @buf1_max_frame: Output - maximum sample frame size for first buffer
 * @buf2_max_frame: Output - maximum sample frame size for second buffer
 * @funcs: Caller provided array to store transform functions to.  It should
 *   have at least #IPATCH_SAMPLE_MAX_TRANSFORM_FUNCS elements.
 * 
 * Get transform function array for converting from @src_format to
 * @dest_format.
 *
 * Returns: Count of function pointers stored to @funcs.  Can be 0 if no
 *   transform is required.
 */
guint
ipatch_sample_get_transform_funcs (int src_format, int dest_format,
                                   guint32 channel_map,
				   guint *buf1_max_frame,
				   guint *buf2_max_frame,
				   IpatchSampleTransformFunc *funcs)
{
  int swidth, dwidth, curfmt, schan, dchan;
  int func_index = 0;
  int max[2] = { 0 };		/* max frame sizes for buffers */

  g_return_val_if_fail (ipatch_sample_format_verify (src_format), 0);
  g_return_val_if_fail (ipatch_sample_format_verify (dest_format), 0);
  g_return_val_if_fail (funcs != NULL, 0);

  if (buf1_max_frame) *buf1_max_frame = 0;
  if (buf2_max_frame) *buf2_max_frame = 0;

  swidth = src_format & IPATCH_SAMPLE_WIDTH_MASK;
  dwidth = dest_format & IPATCH_SAMPLE_WIDTH_MASK;
  schan = IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT (src_format);
  dchan = IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT (dest_format);
  curfmt = src_format;

  /* init max frame size of first buffer to input frame size */
  max[0] = ipatch_sample_format_size (curfmt);

  /* convert to 4 byte 24 bit data before 3 byte */
  if (G_UNLIKELY (dwidth == IPATCH_SAMPLE_REAL24BIT))
    dwidth = IPATCH_SAMPLE_24BIT;

  /* if 3 byte 24 bit data, convert to native endian 4 byte integers */
  if (G_UNLIKELY (swidth == IPATCH_SAMPLE_REAL24BIT))
  {
    if (IPATCH_SAMPLE_FORMAT_IS_LENDIAN (src_format))
      funcs[func_index++] = IPATCH_SAMPLE_FORMAT_IS_SIGNED (src_format)
	? TFF_sle3bto4b : TFF_ule3bto4b;
    else
      funcs[func_index++] = IPATCH_SAMPLE_FORMAT_IS_SIGNED (src_format)
	? TFF_sbe3bto4b : TFF_ube3bto4b;

    swidth = IPATCH_SAMPLE_24BIT;
    curfmt = (curfmt & ~IPATCH_SAMPLE_WIDTH_MASK) | IPATCH_SAMPLE_24BIT;

    update_max_size (&max[func_index & 1], curfmt);
  }

  /* if converting from more channels to less */
  if (G_UNLIKELY (dchan < schan))
  {
    if (G_LIKELY (dchan == 1 && schan == 2))    /* Stereo to mono mapping? */
    { /* Stereo to left map? */
      if (IPATCH_SAMPLE_MAP_GET_CHANNEL (channel_map, 0) == 0)
        funcs[func_index++] = stereo_to_left_tff[swidth - 1];
      else      /* Stereo to right */
        funcs[func_index++] = stereo_to_right_tff[swidth - 1];
    }
    else funcs[func_index++] = chanmap_tff[swidth - 1];

    curfmt = (curfmt & ~IPATCH_SAMPLE_CHANNEL_MASK) | (dchan - 1);
    update_max_size (&max[func_index & 1], curfmt);
  }

  /* source format differs from host byte order? - swap
     REAL24BIT is already swapped by 3 to 4 byte conversion above */
  if (G_UNLIKELY
      (IPATCH_SAMPLE_FORMAT_IS_LENDIAN (src_format)
       != (G_BYTE_ORDER == G_LITTLE_ENDIAN) && swap_tff[swidth - 1]
       && (src_format & IPATCH_SAMPLE_WIDTH_MASK) != IPATCH_SAMPLE_REAL24BIT))
  {
    funcs[func_index++] = swap_tff[swidth - 1];

    curfmt ^= IPATCH_SAMPLE_BENDIAN;
    update_max_size (&max[func_index & 1], curfmt);
  }

  /* if src and dest aren't floating point and sign differs - toggle sign */
  if (G_UNLIKELY
      (swidth != IPATCH_SAMPLE_FLOAT && swidth != IPATCH_SAMPLE_DOUBLE
      && dwidth != IPATCH_SAMPLE_FLOAT && dwidth != IPATCH_SAMPLE_DOUBLE
      && IPATCH_SAMPLE_FORMAT_IS_SIGNED (src_format)
      != IPATCH_SAMPLE_FORMAT_IS_SIGNED (dest_format)))
  {
    if (G_UNLIKELY (swidth == IPATCH_SAMPLE_24BIT))
      funcs[func_index++] = IPATCH_SAMPLE_FORMAT_IS_SIGNED (src_format)
	? TFF_signtou24 : TFF_unsigntos24;
    else funcs[func_index++] = sign_tff[swidth - 1];

    curfmt ^= IPATCH_SAMPLE_UNSIGNED;
    update_max_size (&max[func_index & 1], curfmt);
  }

  if (G_LIKELY (swidth != dwidth))		/* format differs? */
  {
    if (G_UNLIKELY (IPATCH_SAMPLE_FORMAT_IS_FLOATING (curfmt)))
    {
      if (IPATCH_SAMPLE_FORMAT_IS_SIGNED (dest_format))
	funcs[func_index++] = signed_tff[swidth - 1][dwidth - 1];
      else	/* unsigned */
	funcs[func_index++] = unsigned_tff[swidth - 1][dwidth - 1];
    }
    else
    {
      if (IPATCH_SAMPLE_FORMAT_IS_SIGNED (curfmt)) /* signed transform? */
	funcs[func_index++] = signed_tff[swidth - 1][dwidth - 1];
      else	/* unsigned */
	funcs[func_index++] = unsigned_tff[swidth - 1][dwidth - 1];
    }

    curfmt = (curfmt & ~IPATCH_SAMPLE_WIDTH_MASK) | dwidth;
    update_max_size (&max[func_index & 1], curfmt);
  }

  /* destination format differs from host byte order? - swap
     REAL24BIT is swapped below */
  if (G_UNLIKELY
      (IPATCH_SAMPLE_FORMAT_IS_LENDIAN (dest_format)
       != (G_BYTE_ORDER == G_LITTLE_ENDIAN) && swap_tff[dwidth - 1]
       && (dest_format & IPATCH_SAMPLE_WIDTH_MASK) != IPATCH_SAMPLE_REAL24BIT))
  {
    funcs[func_index++] = swap_tff[dwidth - 1];

    curfmt ^= IPATCH_SAMPLE_BENDIAN;
    update_max_size (&max[func_index & 1], curfmt);
  }

  /* if converting from less channels to more */
  if (G_UNLIKELY (dchan > schan))
  {
    if (G_LIKELY (dchan == 2 && schan == 1))    /* Mono to stereo mapping? */
      funcs[func_index++] = mono_to_stereo_tff[dwidth - 1];
    else funcs[func_index++] = chanmap_tff[dwidth - 1]; /* Arbitrary channel mapping */

    curfmt = (curfmt & ~IPATCH_SAMPLE_CHANNEL_MASK) | (dchan - 1);
    update_max_size (&max[func_index & 1], curfmt);
  }

  /* OPTME - Could create channel transform funcs for real 24 bit */

  /* if destination is 3 byte 24 bit data then convert to it */
  if (G_UNLIKELY
      ((dest_format & IPATCH_SAMPLE_WIDTH_MASK) == IPATCH_SAMPLE_REAL24BIT))
  {
    if (IPATCH_SAMPLE_FORMAT_IS_LENDIAN (dest_format))
      funcs[func_index++] = IPATCH_SAMPLE_FORMAT_IS_SIGNED (src_format)
	? TFF_4btosle3b : TFF_4btoule3b;
    else
      funcs[func_index++] = IPATCH_SAMPLE_FORMAT_IS_SIGNED (src_format)
	? TFF_4btosbe3b : TFF_4btoube3b;

    curfmt = (curfmt & ~IPATCH_SAMPLE_WIDTH_MASK) | IPATCH_SAMPLE_REAL24BIT;
    update_max_size (&max[func_index & 1], curfmt);
  }

  if (buf1_max_frame) *buf1_max_frame = max[0];
  if (buf2_max_frame) *buf2_max_frame = max[1];

  return (func_index);
}

static inline void
update_max_size (int *curmax, int format)
{
  int bytewidth;

  bytewidth = ipatch_sample_format_size (format);
  if (bytewidth > *curmax) *curmax = bytewidth;
}
