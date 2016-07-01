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
#ifndef __SAMPLE_H__
#define __SAMPLE_H__

#include <glib.h>

/**
 * IPATCH_SAMPLE_MAX_TRANSFORM_FUNCS: (skip)
 *
 * Maximum number of transform functions returned by
 * ipatch_sample_get_transform_funcs().  Is larger than current actual maximum
 * to allow for future backwards compatible expansion (8 is the real current
 * maximum).
 */
#define IPATCH_SAMPLE_MAX_TRANSFORM_FUNCS	16

#include <libinstpatch/IpatchSampleTransform.h>

/**
 * IPATCH_SAMPLE_FORMAT_MASK:
 *
 * Mask for all fields of sample format integers (width, sign, endian, channel).
 */
#define IPATCH_SAMPLE_FORMAT_MASK   0x1FF

/**
 * IPATCH_SAMPLE_FORMAT_BITCOUNT:
 *
 * Number of bits used for sample format integers.
 */
#define IPATCH_SAMPLE_FORMAT_BITCOUNT  9


#define IPATCH_SAMPLE_WIDTH_MASK    0x00F  /* total of 16 formats (8 reserved) */
#define IPATCH_SAMPLE_CHANNEL_MASK  0x070  /* channel count (8 channels max) */
#define IPATCH_SAMPLE_SIGN_MASK     0x080  /* sign or unsigned (for PCM formats) */
#define IPATCH_SAMPLE_ENDIAN_MASK   0x100  /* endian byte order */

#define IPATCH_SAMPLE_WIDTH_SHIFT   0
#define IPATCH_SAMPLE_CHANNEL_SHIFT 4
#define IPATCH_SAMPLE_SIGN_SHIFT    7
#define IPATCH_SAMPLE_ENDIAN_SHIFT  8

/**
 * IpatchSampleWidth:
 * @IPATCH_SAMPLE_INVALID: Invalid format (so 0 can be used to indicate a NULL state)
 * @IPATCH_SAMPLE_8BIT: 8 bit integer PCM
 * @IPATCH_SAMPLE_16BIT: 16 bit integer PCM
 * @IPATCH_SAMPLE_24BIT: 24 bit integer PCM (32 bit ints)
 * @IPATCH_SAMPLE_32BIT: 32 bit integer PCM
 * @IPATCH_SAMPLE_FLOAT: 32 bit IEEE float (-1.0 - 1.0)
 * @IPATCH_SAMPLE_DOUBLE: 64 bit IEEE double (-1.0 - 1.0)
 * @IPATCH_SAMPLE_REAL24BIT: Real 3 byte 24 bit data (not padded to 32 bits)
 *
 * Sample data widths/formats.
 */
typedef enum
{
  IPATCH_SAMPLE_INVALID = 0,
  IPATCH_SAMPLE_BIT8    = 1,
  IPATCH_SAMPLE_BIT16   = 2,
  IPATCH_SAMPLE_BIT24   = 3,
  IPATCH_SAMPLE_BIT32   = 4,
  IPATCH_SAMPLE_FLOAT   = 5,
  IPATCH_SAMPLE_DOUBLE  = 6,
  IPATCH_SAMPLE_REAL24BIT = 7
} IpatchSampleWidth;

/* Renamed to be GObject Introspection friendly (symbols can't start with numbers).
 * Backwards compatible defines below. */

/**
 * IPATCH_SAMPLE_8BIT: (skip)
 */
#define IPATCH_SAMPLE_8BIT      IPATCH_SAMPLE_BIT8

/**
 * IPATCH_SAMPLE_16BIT: (skip)
 */
#define IPATCH_SAMPLE_16BIT     IPATCH_SAMPLE_BIT16

/**
 * IPATCH_SAMPLE_24BIT: (skip)
 */
#define IPATCH_SAMPLE_24BIT     IPATCH_SAMPLE_BIT24

/**
 * IPATCH_SAMPLE_32BIT: (skip)
 */
#define IPATCH_SAMPLE_32BIT     IPATCH_SAMPLE_BIT32

/**
 * IpatchSampleChannel:
 * @IPATCH_SAMPLE_MONO: Mono audio
 * @IPATCH_SAMPLE_STEREO: Stereo audio
 *
 * Descriptive enums for common audio channel configurations.  These values
 * are actually channel count - 1 (0 = mono, 1 = stereo, etc) and can be compared
 * with the macro IPATCH_SAMPLE_FORMAT_GET_CHANNELS().
 */
typedef enum
{
  IPATCH_SAMPLE_MONO    = 0 << IPATCH_SAMPLE_CHANNEL_SHIFT,
  IPATCH_SAMPLE_STEREO  = 1 << IPATCH_SAMPLE_CHANNEL_SHIFT
} IpatchSampleChannel;

/**
 * IpatchSampleChannelType:
 * @IPATCH_SAMPLE_LEFT: Left channel comes first
 * @IPATCH_SAMPLE_RIGHT: Right channel comes second
 *
 * Channel designation.  Currently there are only 2 designated channels,
 * though the remaining 6 supported channels may be defined to be surround
 * sound channels in the future.
 */
typedef enum
{
  IPATCH_SAMPLE_LEFT =  0,
  IPATCH_SAMPLE_RIGHT = 1
} IpatchSampleChannelType;

/**
 * IPATCH_SAMPLE_MAX_CHANNELS:
 *
 * Maximum number of audio channels handled by libInstPatch.
 */
#define IPATCH_SAMPLE_MAX_CHANNELS      8

/**
 * IpatchSampleSign:
 * @IPATCH_SAMPLE_SIGNED: Signed PCM audio data.
 * @IPATCH_SAMPLE_UNSIGNED: Unsigned PCM audio data.
 *
 * Defines the sign of PCM integer audio data.
 */
typedef enum
{
  IPATCH_SAMPLE_SIGNED   = 0 << IPATCH_SAMPLE_SIGN_SHIFT,
  IPATCH_SAMPLE_UNSIGNED = 1 << IPATCH_SAMPLE_SIGN_SHIFT
} IpatchSampleSign;

/**
 * IpatchSampleEndian:
 * @IPATCH_SAMPLE_LENDIAN: Little endian byte order
 * @IPATCH_SAMPLE_BENDIAN: Big endian byte order
 *
 * Defines the byte order of multi-byte audio data.
 */
typedef enum
{
  IPATCH_SAMPLE_LENDIAN = 0 << IPATCH_SAMPLE_ENDIAN_SHIFT,
  IPATCH_SAMPLE_BENDIAN = 1 << IPATCH_SAMPLE_ENDIAN_SHIFT
} IpatchSampleEndian;

/**
 * IPATCH_SAMPLE_ENDIAN_HOST:
 *
 * Host byte order value (#IPATCH_SAMPLE_LENDIAN or #IPATCH_SAMPLE_BENDIAN).
 */
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define IPATCH_SAMPLE_ENDIAN_HOST IPATCH_SAMPLE_LENDIAN
#else
#define IPATCH_SAMPLE_ENDIAN_HOST IPATCH_SAMPLE_BENDIAN
#endif

/**
 * IPATCH_SAMPLE_FORMAT_GET_WIDTH:
 * @format: Sample format integer
 *
 * Get #IpatchSampleWidth enum from a sample format integer.
 *
 * Returns: Format field of sample format integer.
 */
#define IPATCH_SAMPLE_FORMAT_GET_WIDTH(format) \
  ((format) & IPATCH_SAMPLE_WIDTH_MASK)

/**
 * IPATCH_SAMPLE_FORMAT_IS_FLOATING:
 * @format: Sample format integer
 *
 * Check if a sample format integer defines floating point audio.
 *
 * Returns: %TRUE if sample format is #IPATCH_SAMPLE_FLOAT or #IPATCH_SAMPLE_DOUBLE.
 */
#define IPATCH_SAMPLE_FORMAT_IS_FLOATING(format) \
  (((format) & IPATCH_SAMPLE_WIDTH_MASK) == IPATCH_SAMPLE_FLOAT \
   || ((format) & IPATCH_SAMPLE_WIDTH_MASK) == IPATCH_SAMPLE_DOUBLE)

/**
 * IPATCH_SAMPLE_FORMAT_GET_CHANNELS:
 * @format: Sample format integer
 *
 * Get the channel field from a sample format integer.
 *
 * Returns: Channel field value (see #IpatchSampleChannel)
 */
#define IPATCH_SAMPLE_FORMAT_GET_CHANNELS(format) \
  ((format) & IPATCH_SAMPLE_CHANNEL_MASK)

/**
 * IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT:
 * @format: Sample format integer
 *
 * Get the channel count from a sample format integer.
 *
 * Returns: Channel count (starting at 1 for mono).
 */
#define IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT(format) \
  ((((format) & IPATCH_SAMPLE_CHANNEL_MASK) >> IPATCH_SAMPLE_CHANNEL_SHIFT) + 1)

/**
 * IPATCH_SAMPLE_FORMAT_IS_SIGNED:
 * @format: Sample format integer
 *
 * Check if a sample format integer defines signed audio.
 *
 * Returns: %TRUE if sample format is signed.
 */
#define IPATCH_SAMPLE_FORMAT_IS_SIGNED(format) \
  (((format) & IPATCH_SAMPLE_UNSIGNED) == 0)

/**
 * IPATCH_SAMPLE_FORMAT_IS_UNSIGNED:
 * @format: Sample format integer
 *
 * Check if a sample format integer defines unsigned audio.
 *
 * Returns: %TRUE if sample format is unsigned.
 */
#define IPATCH_SAMPLE_FORMAT_IS_UNSIGNED(format) \
  (((format) & IPATCH_SAMPLE_UNSIGNED) != 0)

/**
 * IPATCH_SAMPLE_FORMAT_IS_LENDIAN:
 * @format: Sample format integer
 *
 * Check if a sample format integer defines little endian audio.
 *
 * Returns: %TRUE if sample format is little endian.
 */
#define IPATCH_SAMPLE_FORMAT_IS_LENDIAN(format) \
  (((format) & IPATCH_SAMPLE_BENDIAN) == 0)

/**
 * IPATCH_SAMPLE_FORMAT_IS_BENDIAN:
 * @format: Sample format integer
 *
 * Check if a sample format integer defines big endian audio.
 *
 * Returns: %TRUE if sample format is big endian.
 */
#define IPATCH_SAMPLE_FORMAT_IS_BENDIAN(format) \
  (((format) & IPATCH_SAMPLE_BENDIAN) != 0)

/**
 * ipatch_sample_format_size:
 * @format: Sample format integer
 *
 * Get frame byte size for a given sample format (sample byte size * channels).
 *
 * Returns: Size in bytes of a single sample frame.
 */
#define ipatch_sample_format_size(format) \
  (ipatch_sample_width_sizes[(format) & IPATCH_SAMPLE_WIDTH_MASK] \
   * IPATCH_SAMPLE_FORMAT_GET_CHANNEL_COUNT (format))

/**
 * ipatch_sample_format_width:
 * @format: Sample format
 *
 * Gets the number of bytes used for storing a single sample for @format.
 * Doesn't take into account channels.  This is the number of bytes used
 * to store the samples, not the effective bit width.  For example:
 * #IPATCH_SAMPLE_24BIT uses 4 bytes for each sample.
 *
 * Returns: Byte width of a single sample (not including channels).
 */
#define ipatch_sample_format_width(format) \
  (ipatch_sample_width_sizes[(format) & IPATCH_SAMPLE_WIDTH_MASK])


/**
 * IPATCH_SAMPLE_MAP_CHANNEL:
 * @dest: Destination channel in mapping bit field (0-7)
 * @src: Source channel (0-7)
 *
 * Macro to calculate a channel mapping value for a given destination and
 * source.  A channel mapping is composed of up to 24 bits
 * (3 bits * 8 channels = 24).  Channel mappings are used for sample
 * conversions to route channels from a source format to a destination format.
 * Multiple channel map values should be OR'd together.
 */
#define IPATCH_SAMPLE_MAP_CHANNEL(dest, src)   ((src) << (3 * (dest)))

/**
 * IPATCH_SAMPLE_MAP_GET_CHANNEL:
 * @map: Channel map value (#guint32 - only 24 bits are used)
 * @dest: Destination channel in @map bit field (0-7)
 *
 * Macro to get a source channel value given a destination channel.
 *
 * Returns: Source channel for @dest (0-7)
 */
#define IPATCH_SAMPLE_MAP_GET_CHANNEL(map, dest)   (((map) >> ((dest) * 3)) & 0x07)

/**
 * IPATCH_SAMPLE_UNITY_CHANNEL_MAP:
 *
 * Unity channel mapping which routes each input channel to the same output
 * channel.
 */
#define IPATCH_SAMPLE_UNITY_CHANNEL_MAP   0xFAC688


extern guint ipatch_sample_width_sizes[16];

int ipatch_sample_format_bit_width (int format);
gboolean ipatch_sample_format_verify (int format);
gboolean ipatch_sample_format_transform_verify (int src_format, int dest_format,
                                                guint32 channel_map);
guint ipatch_sample_get_transform_funcs (int src_format, int dest_format,
					 guint32 channel_map,
					 guint *buf1_max_frame,
					 guint *buf2_max_frame,
					 IpatchSampleTransformFunc *funcs);
#endif
