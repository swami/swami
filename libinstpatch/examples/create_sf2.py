#!/usr/bin/python

# Python example of creating a SoundFont from multiple audio samples
#
# Updated on July 22, 2006
# Now uses more convenient converter functions.
#
# Josh Green - Sep 13, 2004
# Use this example as you please (public domain)
#
# Note that the Python binding API is not stable yet and may change
#

import ipatch
import gobject, sys

if len (sys.argv[1:]) == 0:
  print "Usage: create_sf2.py sample1.wav [sample2.wav sample3.aiff ...]"
  sys.exit ()

sf2 = ipatch.SF2 ()		# Create new SoundFont object

# For each file specified on command line..
for fname in sys.argv[1:]:
  # Identify and open the file
  f = ipatch.ipatch_file_identify_open (fname, "r")

  if not f:
    print "Failed to identify file '%s'" % fname
    continue

  # Convert sample file to SF2Sample object
  sample = ipatch.ipatch_convert_object_to_type (f, ipatch.SF2Sample)
  if not sample: continue

  sf2.add_unique (sample)		# Add sample and ensure name is unique

  name = sample.get_property ("name")	# Get the sample name

  inst = ipatch.SF2Inst ()		# Create new SoundFont instrument
  inst.set_property ("name", name)	# Set the instrument name = sample name
  inst.new_zone (sample)		# Add new zone to inst => sample
  sf2.add_unique (inst)			# Add inst and ensure name is unique

  preset = ipatch.SF2Preset ()		# Create new preset
  preset.set_property ("name", name)	# Set preset name = sample name
  preset.new_zone (inst)		# Add zone to preset => inst
  sf2.add_unique (preset)		# Add preset, name/bank/preset # unique

# Create SoundFont file object, set its name and open for writing
sffile = ipatch.SF2File ()
sffile.open ("output.sf2", "w")

ipatch.ipatch_convert_objects (sf2, sffile)

sffile.close ()
