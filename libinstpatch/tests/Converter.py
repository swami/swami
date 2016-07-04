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

parser = Test.createArgParser ('libInstPatch Converter tests')
args = Test.parseArgs (parser)

Ipatch.init ()

convinfo = [Ipatch.get_converter_info (t) for t in Ipatch.find_converters (GObject.TYPE_NONE, GObject.TYPE_NONE)]
convinfo.sort (key=lambda info: GObject.type_name (info.conv_type))

Test.info ("Registered converters:\n")

srcTypeCountHash = {}
destTypeCountHash = {}

for conv in convinfo:
  s = GObject.type_name (conv.conv_type) + "\n"

  if conv.src_count == Ipatch.ConverterCount.ONE_OR_MORE: count = '+'
  elif conv.src_count == Ipatch.ConverterCount.ZERO_OR_MORE: count = '*'
  else: count = str (conv.src_count)

  if conv.src_match == GObject.TYPE_INVALID:
    s += "src[%s]=%s " % (count, GObject.type_name (conv.src_type))
  else: s += "src[%s]=%s - %s" \
    % (count, GObject.type_name (conv.src_match), GObject.type_name (conv.src_type))

  if conv.flags & Ipatch.ConverterInfoFlags.DERIVED:
    s += "+ DESCENDANTS\n"
  else: s += "\n"

  if conv.dest_count == Ipatch.ConverterCount.ONE_OR_MORE: count = '+'
  elif conv.dest_count == Ipatch.ConverterCount.ZERO_OR_MORE: count = '*'
  else: count = str (conv.dest_count)

  if conv.dest_match == GObject.TYPE_INVALID:
    s += "dst[%s]=%s\n" % (count, GObject.type_name (conv.dest_type))
  else: s += "dst[%s]=%s - %s\n" \
    % (count, GObject.type_name (conv.dest_match), GObject.type_name (conv.dest_type))

  Test.info (s)

  srctype = conv.src_match if conv.src_match != GObject.TYPE_INVALID else conv.src_type
  count = srcTypeCountHash.get (srctype, 0) + 1
  srcTypeCountHash[srctype] = count

  desttype = conv.dest_match if conv.dest_match != GObject.TYPE_INVALID else conv.dest_type
  count = destTypeCountHash.get (desttype, 0) + 1
  destTypeCountHash[desttype] = count


if len (convinfo) == 0:
  Test.error ("No converters found!")

Test.msg ("Found %d converters\n" % len (convinfo))

# Display source and destination type count summaries
Test.msg ("Source type summary:")
srcTypeCounts = sorted ([GObject.type_name (t) + ":" + str (c) for t, c in srcTypeCountHash.items ()])
Test.msg ("\n".join (srcTypeCounts))

Test.msg ("\nDestination type summary:")
destTypeCounts = sorted ([GObject.type_name (t) + ":" + str (c) for t, c in destTypeCountHash.items ()])
Test.msg ("\n".join (destTypeCounts))

Test.exit ()

