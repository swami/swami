#!/usr/bin/python
#
# Project: lib Instrument Patch Python testing suite
# File:    Testing.py
# Descr:   Testing support functions used by other tests
# Author:  Element Green <element@elementsofsound.org>
# Date:    2016-07-01
# License: Public Domain
#
import sys
import os
import argparse
import gi
from gi.repository import GLib
from gi.repository import GObject

gi.require_version ('Ipatch', '1.1')
from gi.repository import Ipatch

ErrorCount = 0
LogMask = 0
Verbose = 1
TEST_DATA_PATH = "test-data/"

def createArgParser (description):
  parser = argparse.ArgumentParser (description=description)
  parser.add_argument ('-v', '--verbose', action='count', help='Increase verbosity')
  parser.add_argument ('--quiet', action='store_true', help='Only display errors')
  return parser

def parseArgs (parser):
  global Verbose

  args = parser.parse_args ()

  if args.verbose > 0:
    Verbose = args.verbose + 1

  if args.quiet:
    Verbose = 0

  return args

def LogTrap (log_domain, log_level, message, user_data):
  global LogMask

  LogMask |= log_level
  GLib.log_default_handler ("Testing", log_level, message, 0)

def exit ():
  if LogMask: print ("GLib warning/error messages occurred!")
  if ErrorCount > 0: print ("%d Errors occurred!" % ErrorCount)
  sys.exit (0 if ErrorCount == 0 and LogMask == 0 else 1)

def info (s):
  if Verbose >= 2:
    print (s)

def msg (s):
  if Verbose >= 1:
    print (s)

def error (s):
  global ErrorCount

  print (s)
  ErrorCount += 1

def compareItems (item1, item2, recurse=True, blackList=()):
  """Compare two Ipatch.Item objects"""

  if type (item1) != type (item2):
    raise RuntimeError ('Item type %s != %s' % (str (type (item1)), str (type (item2))))

  typename = str (type (item1))
  props = sorted ([prop for prop in item1.list_properties ()], key=lambda prop: prop.name)

  blprops = [propname for typ, propname in blackList if isinstance (item1, typ)]

  for p in props:
    if p.name in blprops: continue

    val1 = item1.get_property (p.name)
    val2 = item2.get_property (p.name)

    valtype = p.value_type

    if val1 is None != val2 is None:
      raise RuntimeError ('Property value None mismatch for property %s:%s'
                          % (typename, p.name))

    if valtype.is_a (GObject.Object):
      if val1 is not None and recurse:
        compareItems (val1, val2, recurse=False, blackList=blackList)   # Don't recurse into object fields (causes recursion loops)
    elif valtype.is_a (Ipatch.Range):
      if val1.low != val2.low or val1.high != val2.high:
        RuntimeError ('Range value mismatch for property %s:%s, %d-%d != %d-%d'
                      % (typename, p.name, val1.low, val1.high, val2.low, val2.high))
    elif valtype.is_a (GObject.GBoxed):
      if valtype.name == 'IpatchSF2ModList':
        if len (val1) != len (val2):
          RuntimeError ('Modulator list length mismatch for property %s:%s, %d != %d'
                        % (typename, p.name, len (val1), len (val2)))

        for i in xrange (0, len (val1)):
          v1 = val1[i]
          v2 = val2[i]

          v1 = (v1.src, v1.amtsrc, v1.dest, v1.amount, v1.trans)
          v2 = (v2.src, v2.amtsrc, v2.dest, v2.amount, v2.trans)

          if v1 != v2:
            RuntimeError ('Modulator value mismatch for property %s:%s, %s != %s'
                          % (typename, p.name, str (v1), str (v2)))
      else:
        print ('Ignoring property %s:%s of type %s'
               % (typename, p.name, str (valtype)))
    elif val1 != val2:
      raise RuntimeError ('Value mismatch for property %s:%s, %s != %s'
                          % (typename, p.name, str (val1), str (val2)))

  if recurse and isinstance (item1, Ipatch.Container):
    children1 = item1.get_children ()
    children2 = item2.get_children ()

    if len (children1) != len (children2):
      raise RuntimeError ('Container of type %s has unequal children count, %d != %d'
                          % (typename, len (children1), len (children2)))

    for i in xrange (0, len (children1)):
      compareItems (children1[i], children2[i], blackList=blackList)

# Log flags we want to trap on
flags = GLib.LogLevelFlags.LEVEL_WARNING | GLib.LogLevelFlags.LEVEL_ERROR \
 | GLib.LogLevelFlags.LEVEL_CRITICAL | GLib.LogLevelFlags.FLAG_FATAL | GLib.LogLevelFlags.FLAG_RECURSION

GLib.log_set_handler (None, flags, LogTrap, None)

# Create test data directory if needed
try: 
  os.makedirs (TEST_DATA_PATH)
except OSError:
  if not os.path.isdir (TEST_DATA_PATH):
    raise

