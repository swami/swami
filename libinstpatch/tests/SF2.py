#!/usr/bin/python
#
# Project: lib Instrument Patch Python testing suite
# File:    SF2.py
# Descr:   SoundFont tests
# Author:  Element Green <element@elementsofsound.org>
# Date:    2016-07-01
# License: Public Domain
#
import Test
import gi
import ctypes
import math
import sys
import os

from gi.repository import GObject

gi.require_version ('Ipatch', '1.1')
from gi.repository import Ipatch

# Sample audio format (native SoundFont 16 bit mono signed little endian byte order audio)
SAMPLE_FORMAT = Ipatch.SampleWidth.BIT16 | Ipatch.SampleChannel.MONO | Ipatch.SampleSign.SIGNED | Ipatch.SampleEndian.LENDIAN

# Format used for writing sample data (converted from this format)
ACCESS_FORMAT = Ipatch.SampleWidth.FLOAT | Ipatch.SampleChannel.MONO | Ipatch.SampleSign.SIGNED
SAMPLE_RATE             = 48000         # Sample rate
SAMPLE_LOOP_PADDING     = 4             # Number of samples before and after the sample loop
TEST_SF2_FILENAME       = os.path.join (Test.TEST_DATA_PATH, 'test.sf2')

# Set sample access format endian byte order depending on host system
ACCESS_FORMAT |= Ipatch.SampleEndian.BENDIAN if sys.byteorder == 'big' else Ipatch.SampleEndian.LENDIAN

def SineWave (periodPos):
  """Sine waveform generation function"""
  return math.sin (periodPos * 2 * math.pi)

def SawWave (periodPos):
  """Triangle waveform generation function"""
  if (periodPos <= 0.25): return periodPos / 0.25
  elif (periodPos <= 0.75): return (0.75 - periodPos) * 4.0 - 1.0
  else: return (1.0 - periodPos) * -4.0

def SquareWave (periodPos):
  """Square waveform generation function"""
  return 1.0 if periodPos <= 0.5 else 0.0

def sampleDataGen (freq, mathfunc):
  """Raw sample data generator"""

  period = float (SAMPLE_RATE) / freq

  start = int (math.floor (period - SAMPLE_LOOP_PADDING))
  end = int (math.ceil (2 * period + SAMPLE_LOOP_PADDING))
  size = end - start

  buf = (ctypes.c_float * size)()

  # Generate a single period of the waveform, including loop padding
  for i in xrange (0, size):
    periodPos = math.fmod (start + i, period) / period
    buf[i] = mathfunc (periodPos)

  return bytearray (buf)

def createSF2Sample (floatData):
  """Generate sample data object"""

  sampleSize = len (floatData) / 4

  # Create RAM sample store then set format, rate, and size
  store = Ipatch.SampleStoreRam ()
  store.set_properties (sample_format=SAMPLE_FORMAT, sample_rate=SAMPLE_RATE, sample_size=sampleSize)

  # Open a handle to the sample store and write the waveform floating point data to it
  retval, handle = store.handle_open ('w', ACCESS_FORMAT, 0)
  handle.write (0, floatData)   # 0 is the position to write to (start of sample)
  handle.close ()

  # Create sample data object and add sample store to it
  sampleData = Ipatch.SampleData ()
  sampleData.add (store)

  # Create SF2Sample and add sample data to it
  sf2Sample = Ipatch.SF2Sample ()
  sf2Sample.set_data (sampleData)

  loopStart = SAMPLE_LOOP_PADDING
  loopEnd = sampleSize - SAMPLE_LOOP_PADDING - 1

  # Calculate root note and fine tune based on loop length for a single waveform
  notecents = Ipatch.unit_hertz_to_cents (float (SAMPLE_RATE) / (loopEnd - loopStart))
  cents = math.fmod (notecents, 100.0)

  # Fine tune offset corrects for inaccuracy of a single digital waveform
  if cents <= 50.0:
    rootNote = int (notecents / 100.0)
    fineTune = -int (round (cents))
  else:
    rootNote = int (notecents / 100.0) + 1
    fineTune = int (round (100.0 - cents))

  sf2Sample.set_properties (sample_rate=SAMPLE_RATE, loop_start=loopStart, loop_end=loopEnd,
                            root_note=rootNote, fine_tune=fineTune)
  return sf2Sample

def addSF2Preset (sf2, name, midiProgram, sampleMathFunc):
  """Create a SoundFont preset, with an instrument, and containing several generated samples"""

  # Create a SF2 preset object, set its MIDI bank and program numbers (MIDI locale), and add it to the SF2 object
  preset = Ipatch.SF2Preset ()
  preset.set_name (name)
  preset.set_midi_locale (0, midiProgram)
  sf2.add (preset)

  # Create a SF2 instrument object and add it to the SF2 object
  inst = Ipatch.SF2Inst ()
  inst.set_name (name)
  sf2.add (inst)
  preset.new_zone (inst)        # Create new preset zone and add instrument to it

  # Create 2 modulators to disable the default pitch modulation control and modulate the filter cutoff instead
  mod = Ipatch.SF2Mod ()
  mod.src = 1 | Ipatch.SF2ModControlPalette.MIDI | Ipatch.SF2ModDirection.POSITIVE \
    | Ipatch.SF2ModPolarity.UNIPOLAR | Ipatch.SF2ModType.LINEAR
  mod.dest = Ipatch.SF2GenType.VIB_LFO_TO_PITCH
  mod.amount = 0
  mod.amtsrc = 0
  mod.trans = Ipatch.SF2ModTransform.LINEAR

  modlist = [mod]       # Create modulator list

  mod = Ipatch.SF2Mod ()
  mod.src = 1 | Ipatch.SF2ModControlPalette.MIDI | Ipatch.SF2ModDirection.POSITIVE \
    | Ipatch.SF2ModPolarity.UNIPOLAR | Ipatch.SF2ModType.LINEAR
  mod.dest = Ipatch.SF2GenType.MOD_LFO_TO_FILTER_CUTOFF
  mod.amount = 2400
  mod.amtsrc = 0
  mod.trans = Ipatch.SF2ModTransform.LINEAR

  modlist.append (mod)  # Append 2nd modulator to list

  # Set instrument global modulators to modulator list
  inst.set_mods (modlist)

  # Create A note samples for octaves A0 through A6 (7 octaves) and add to instrument zones
  for octave in xrange (0, 7):
    note = 33 + octave * 12     # MIDI note 33 is A0
    sample = createSF2Sample (sampleDataGen (Ipatch.unit_cents_to_hertz (note * 100), sampleMathFunc))
    sample.set_name ('%s A%d' % (name, octave))
    sf2.append (sample)         # Add sample to SF2 base object

    # Create instrument zone
    izone = Ipatch.SF2IZone ()
    inst.add (izone)

    izone.set_sample (sample)   # Assign the sample to the zone

    # Calculate optimal note ranges for samples
    start = note - 5 if octave > 0 else 0
    end = note + 6 if octave < 6 else 127

    izone.set_properties (loop_type=Ipatch.SampleLoopType.STANDARD,             # Enable looping
                          note_range=Ipatch.Range.new (start, end))             # Create range and assign to note range

def createSF2 ():
  """Create SoundFont object"""

  # Create SoundFont object, a file object, and assign it
  sf2 = Ipatch.SF2 ()
  sf2.props.name = "Test SoundFont"
  sf2.props.author = "Robo Python 3000+"
  sf2.props.comment = """This is a test SoundFont generated by the Instrument Patch library Python binding.
One should find 3 different presets for Sine, Saw, and Square waves with 7 samples each.
Enjoy!"""

  sf2File = Ipatch.SF2File ()
  sf2File.set_name (TEST_SF2_FILENAME)
  sf2.set_file (sf2File)

  # Add presets for sine, saw, and square waves
  addSF2Preset (sf2, "Sine", 0, SineWave)
  addSF2Preset (sf2, "Saw", 1, SawWave)
  addSF2Preset (sf2, "Square", 2, SquareWave)

  return sf2


# Main
if __name__ == "__main__":
  parser = Test.createArgParser ('libInstPatch SoundFont tests')
  parser.add_argument ('-n', '--nodelete', action='store_true', help="Don't delete created test SoundFont (%s)" % TEST_SF2_FILENAME)
  args = Test.parseArgs (parser)

  Ipatch.init ()

  Test.msg ("Creating SoundFont")
  sf2 = createSF2 ()

  Test.msg ("Saving SoundFont")
  sf2.save ()

  Test.msg ("Loading SoundFont")
  sf2file = Ipatch.SF2File ()
  sf2file.set_property ('file-name', TEST_SF2_FILENAME)
  cmpsf2 = Ipatch.convert_object_to_type (sf2file, Ipatch.SF2)

  blacklistProps = {
    (Ipatch.SF2, "software"),             # SF2 software will include the loaded software version appended
    (Ipatch.SF2File, "sample-size")       # Sample size is the total sample data in the SoundFont file which will differ
  }

  # Clear changed and saved flags so they don't trigger a value mismatch
  sf2.clear_flags (Ipatch.BaseFlags.CHANGED | Ipatch.BaseFlags.SAVED)
  cmpsf2.clear_flags (Ipatch.BaseFlags.CHANGED | Ipatch.BaseFlags.SAVED)

  Test.msg ("Verifying SoundFont")
  Test.compareItems (sf2, cmpsf2, blackList=blacklistProps)

  if not args.nodelete:
    os.unlink (TEST_SF2_FILENAME)

  Test.exit ()

