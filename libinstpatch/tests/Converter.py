#!/usr/bin/python
#
# Project: lib Instrument Patch Python testing suite
# File:    Converter.py
# Descr:   Object converter tests
# Author:  Element Green <element@elementsofsound.org>
# Date:    2016-07-04
# License: Public Domain
#
import Test
import gi

from gi.repository import GObject

gi.require_version ('Ipatch', '1.1')
from gi.repository import Ipatch

# Common converter source to destination type mappings
CONVERTER_TYPES = (
  ("IpatchFile", "IpatchBase"),
  ("IpatchBase", "IpatchFile"),
  ("IpatchItem", "IpatchSF2VoiceCache"),
  ("IpatchSndFile", "IpatchItem")
)

# Main
if __name__ == "__main__":
  parser = Test.createArgParser ('libInstPatch Converter tests')
  args = Test.parseArgs (parser)

  Ipatch.init ()

  convinfo = [Ipatch.get_converter_info (t) for t in Ipatch.find_converters (GObject.TYPE_NONE, GObject.TYPE_NONE, 0)]
  convinfo.sort (key=lambda info: GObject.type_name (info.conv_type))

  if len (convinfo) == 0:
    Test.error ("No converters found!")

  Test.msg ("Found %d converters\n" % len (convinfo))

  summaryCounts = [0] * len (CONVERTER_TYPES)
  summaryOther = 0

  # Show converter details
  for convindex in xrange (0, len (convinfo)):
    conv = convinfo[convindex]

    s = GObject.type_name (conv.conv_type) + "\n"

    if conv.src_count == Ipatch.ConverterCount.ONE_OR_MORE: count = '+'
    elif conv.src_count == Ipatch.ConverterCount.ZERO_OR_MORE: count = '*'
    else: count = str (conv.src_count)

    if conv.src_match == GObject.TYPE_INVALID:
      s += "  src[%s]=%s " % (count, GObject.type_name (conv.src_type))
    else: s += "  src[%s]=%s - %s" \
      % (count, GObject.type_name (conv.src_match), GObject.type_name (conv.src_type))

    if conv.flags & Ipatch.ConverterFlags.SRC_DERIVED:
      s += "+ DESCENDANTS\n"
    else: s += "\n"

    if conv.dest_count == Ipatch.ConverterCount.ONE_OR_MORE: count = '+'
    elif conv.dest_count == Ipatch.ConverterCount.ZERO_OR_MORE: count = '*'
    else: count = str (conv.dest_count)

    if conv.dest_match == GObject.TYPE_INVALID:
      s += "  dst[%s]=%s" % (count, GObject.type_name (conv.dest_type))
    else: s += "  dst[%s]=%s - %s" \
      % (count, GObject.type_name (conv.dest_match), GObject.type_name (conv.dest_type))

    if conv.flags & Ipatch.ConverterFlags.DEST_DERIVED:
      s += "+ DESCENDANTS"

    Test.info (s)

    for i in xrange (0, len (CONVERTER_TYPES)):
      convTypes = CONVERTER_TYPES[i]

      if GObject.type_is_a (conv.src_type, GObject.type_name (convTypes[0])) \
          and GObject.type_is_a (conv.dest_type, GObject.type_name (convTypes[1])):
        summaryCounts[i] += 1
        break
    else: summaryOther += 1

  Test.info ("")

  Test.msg ("Converter type summary:")

  for i in xrange (0, len (CONVERTER_TYPES)):
    convTypes = CONVERTER_TYPES[i]
    Test.msg ("%s -> %s: %d" % (convTypes[0], convTypes[1], summaryCounts[i]))

  if summaryOther > 0:
    Test.msg ("Other: %d" % (convTypes[0], convTypes[1], summaryCounts[i]))

  Test.exit ()

