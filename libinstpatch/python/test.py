#!/usr/bin/python

import sys, getopt, os.path
import ipatch
import gobject

def display_item_tree (obj, indent=None):
    if indent is None:
        indent = 0

    print "  " * indent + obj.describe () + " <" \
          + gobject.type_name (obj.__gtype__) + ">"
    if isinstance (obj, ipatch.Container):
        for type in obj.child_types ():
            for child in obj.get_children (type):
                display_item_tree (child, indent+1)

def usage ():
    print "Usage: " + script + """ [OPTIONS]... [FILES]...
  -h, --help            Print help
"""
    sys.exit (1)

script = os.path.splitext (os.path.basename (sys.argv[0]))[0]
try:
    (OptList, ArgList) = getopt.getopt (sys.argv[1:],
                                        "", ("help"))
except getopt.error:
    usage()

for i in OptList:
    if i[0] in ("-h", "--help"):
        usage()

if len (ArgList) < 1: usage()

for name in ArgList:
    file = ipatch.File ()
    file.set_name (name)
    object = file.load_object ()
    display_item_tree (object)
