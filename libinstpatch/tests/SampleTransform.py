#!/usr/bin/python
#
# Project: lib Instrument Patch Python testing suite
# File:    SF2.py
# Descr:   SoundFont tests
# Author:  Element Green <element@elementsofsound.org>
# Date:    2016-07-01
# License: Public Domain
#
"""
Convert between all possible audio format pairs using the following steps:
- Convert double to first format of format pair
- Convert from first format to second format
- Convert second format back to double
- Calculate maximum audio sample difference between new double audio and original
"""

import Test
import gi
import ctypes
import math
import sys

from gi.repository import GObject

gi.require_version ('Ipatch', '1.1')
from gi.repository import Ipatch

# Default values are non powers of 2 to decrease the chance of masking periodic data issues
DEFAULT_SAMPLE_SIZE             = 1756          # default test waveform size in samples
DEFAULT_SAMPLE_PERIOD           = 123           # default waveform period in samples
DEFAULT_TRANSFORM_SIZE          = 65000         # default transform buffer size in bytes
MAX_DIFF_ALLOWED                = 0.016         # maximum difference allowed

# all available sample format combinations
testFormats = (
  Ipatch.SampleWidth.BIT8,
  Ipatch.SampleWidth.BIT16,
  Ipatch.SampleWidth.BIT24,
  Ipatch.SampleWidth.BIT32,
  Ipatch.SampleWidth.FLOAT,
  Ipatch.SampleWidth.DOUBLE,
  Ipatch.SampleWidth.REAL24BIT,

  Ipatch.SampleWidth.BIT8 | Ipatch.SampleSign.UNSIGNED,
  Ipatch.SampleWidth.BIT16 | Ipatch.SampleSign.UNSIGNED,
  Ipatch.SampleWidth.BIT24 | Ipatch.SampleSign.UNSIGNED,
  Ipatch.SampleWidth.BIT32 | Ipatch.SampleSign.UNSIGNED,
  Ipatch.SampleWidth.REAL24BIT | Ipatch.SampleSign.UNSIGNED,

  Ipatch.SampleWidth.BIT16 | Ipatch.SampleEndian.BENDIAN,
  Ipatch.SampleWidth.BIT24 | Ipatch.SampleEndian.BENDIAN,
  Ipatch.SampleWidth.BIT32 | Ipatch.SampleEndian.BENDIAN,
  Ipatch.SampleWidth.FLOAT | Ipatch.SampleEndian.BENDIAN,
  Ipatch.SampleWidth.DOUBLE | Ipatch.SampleEndian.BENDIAN,
  Ipatch.SampleWidth.REAL24BIT | Ipatch.SampleEndian.BENDIAN,

  Ipatch.SampleWidth.BIT16 | Ipatch.SampleSign.UNSIGNED | Ipatch.SampleEndian.BENDIAN,
  Ipatch.SampleWidth.BIT24 | Ipatch.SampleSign.UNSIGNED | Ipatch.SampleEndian.BENDIAN,
  Ipatch.SampleWidth.BIT32 | Ipatch.SampleSign.UNSIGNED | Ipatch.SampleEndian.BENDIAN,
  Ipatch.SampleWidth.REAL24BIT | Ipatch.SampleSign.UNSIGNED | Ipatch.SampleEndian.BENDIAN
)

testFormatNames = (
  "8bit-signed",
  "16bit-signed-lendian",
  "24bit-signed-lendian",
  "32bit-signed-lendian",
  "float-lendian",
  "double-lendian",
  "real24bit-signed-lendian",

  "8bit-unsigned",
  "16bit-unsigned-lendian",
  "24bit-unsigned-lendian",
  "32bit-unsigned-lendian",
  "real24bit-unsigned-lendian",

  "16bit-signed-bendian",
  "24bit-signed-bendian",
  "32bit-signed-bendian",
  "float-bendian",
  "double-bendian",
  "real24bit-signed-bendian",

  "16bit-unsigned-bendian",
  "24bit-unsigned-bendian",
  "32bit-unsigned-bendian",
  "real24bit-unsigned-bendian"
)


parser = Test.createArgParser ('libInstPatch audio sample transform tests')
parser.add_argument ( '-s', '--size', type=int, default=DEFAULT_SAMPLE_SIZE, help='Size of audio to transform in frames')
parser.add_argument ('-p', '--period', type=int, default=DEFAULT_SAMPLE_PERIOD, help='Period size in frames')
parser.add_argument ('-t', '--trans', type=int, default=DEFAULT_TRANSFORM_SIZE, help='Transform size in bytes')
args = Test.parseArgs (parser)

Ipatch.init ()

# Determine host endian byte order
hostEndian = Ipatch.SampleEndian.BENDIAN if sys.byteorder == 'big' else Ipatch.SampleEndian.LENDIAN

# Create double floating point sample data audio
sampleSize = args.size
period = args.period
sampleDataArray = (ctypes.c_double * sampleSize)()

for pos in xrange (0, sampleSize):
  periodPos = math.fmod (pos, period) / period
  sampleDataArray[pos] = math.sin (periodPos * 2 * math.pi)

sampleData = bytearray (sampleDataArray)

# Create sample transform object and initialize size
trans = Ipatch.SampleTransform ()
trans.alloc_size (args.trans)

failcount = 0
max_maxdiff = 0.0

for isrc in xrange (0, len (testFormats)):
  srcform = testFormats[isrc]

  for idest in xrange (0, len (testFormats)):
    destform = testFormats[idest]

    # convert generated double floating point waveform to source format
    trans.set_formats (Ipatch.SampleWidth.DOUBLE | hostEndian, srcform,
                       Ipatch.SAMPLE_UNITY_CHANNEL_MAP)
    srcData = trans.convert (sampleData)

    # convert source format to destination format
    trans.set_formats (srcform, destform, Ipatch.SAMPLE_UNITY_CHANNEL_MAP)
    destData = trans.convert (srcData)

    # convert destination format to final double output
    trans.set_formats (destform, Ipatch.SampleWidth.DOUBLE | hostEndian,
                       Ipatch.SAMPLE_UNITY_CHANNEL_MAP)
    finalData = trans.convert (destData)
    finalDataArray = (ctypes.c_double * sampleSize).from_buffer_copy (finalData)

    maxdiff = 0.0
    maxindex = 0

    # compare final waveform against original
    for i in xrange (0, sampleSize):
      d = abs (sampleDataArray[i] - finalDataArray[i])

      if d > maxdiff:
        maxdiff = d
        maxindex = i

    if maxdiff > max_maxdiff:
      max_maxdiff = maxdiff

    if maxdiff <= MAX_DIFF_ALLOWED:
      Test.info ("Converted %s to %s: maxdiff=%0.6f, index=%d"
                 % (testFormatNames[isrc], testFormatNames[idest], maxdiff, maxindex))
    else:
      Test.error ("FAILED Converting %s to %s: maxdiff=%0.6f, index=%d"
                  % (testFormatNames[isrc], testFormatNames[idest], maxdiff, maxindex))
      failcount += 1

if failcount > 0:
  Test.error ("%d of %d format conversions FAILED (max diff=%0.6f)" % (failcount, len (testFormats) ** 2, max_maxdiff))
else: Test.msg ("All %d sample format conversions PASSED (max diff=%0.6f)" % (len (testFormats) ** 2, max_maxdiff))


Test.exit ()

