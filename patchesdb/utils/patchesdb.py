#!/usr/bin/python
#
# patchesdb.py - Command line interface to PatchesDB database
#
# PatchesDB
# Copyright (C) 2005-2007 Josh Green <jgreen@users.sourceforge.net>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; version 2
# of the License only.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA or point your web browser to http://www.gnu.org.
# 
# Following commands supported:
#
# extract [file(s)]
#   Each file specified is extracted to a new directory <file>-extract.
#
# import [path(s)]
#   Imports one or more files and/or directories.  Files are considered to be
#   archives (in which case they are extracted) or raw instrument files.
#   Directories are considered to be extracted archives containing an instrument
#   file and zero or more other arbitrary files.
#
# activate [PatchId(s)]
#   Space separated list of patch IDs to activate
#
# delete [PatchId(s)]
#   Delete one or more patches by ID
#

DEFAULT_USERID = 2

import sys
sys.path += ("..",)         # Add parent directory to module search path

import InstDB
import PatchImport
import os
import Util

InstDB.connect ()

def usage ():
  print "Usage: patchesdb.py [CMD] <PARAMS>"
  print "  extract [file(s)]      Extract one or more files to <file>-extract"
  print "  import [path(s)]       Import one or more directories and/or files"
  print "  activate [PatchId(s)]  Space separated list of patch IDs to activate"
  print "  delete [PatchId(s)]    Space separated list of patch IDs to delete"
  sys.exit (2)

def do_extract (files):
  for fname in files:
    extractDir = fname + '-extract'
    os.mkdir (extractDir)

    print "Extracting '%s' to '%s'" % (fname, extractDir)

    # Handle archive/raw instrument file accordingly
    if not PatchImport.extract_archive (fname, os.path.basename (fname),
                                        extractDir):
        os.rmdir (extractDir)
        raise InstDB.ImportError, "Unknown file type"

def do_import (paths):
  for path in paths:
    if os.path.isfile (path):
      extractDir = path + '-extract'
      os.mkdir (extractDir)

      print "Extracting/copying file '%s' to '%s'" % (path, extractDir)

      # Handle archive/raw instrument file accordingly
      if not PatchImport.extract_archive (fname, os.path.basename (path),
                                          extractDir):
          os.rmdir (extractDir)
          raise InstDB.ImportError, "Unknown file type"

    else: extractDir = path

    print "Importing '%s'" % path

    # Import the instrument information
    PatchId = PatchImport.import_patch (extractDir, DEFAULT_USERID)

    print "Imported '%s' with PatchId=%s" % (path, PatchId)

def do_activate (patchids):
  for id in patchids:
    id = int (id)

    InstDB.Cur.execute ("SELECT FileName FROM PatchInfo WHERE PatchId=%s", id)

    row = InstDB.Cur.fetchone ()
    if not row:
      print "*** Error: patch with Id %d not found!" % id
      continue

    print "Activating patch '%s' with Id %d" % (row["FileName"], id)

    PatchImport.activate_patch (id)

def do_delete (patchids):
  for id in patchids:
    id = int (id)

    InstDB.Cur.execute ("SELECT FileName FROM PatchInfo WHERE PatchId=%s", id)

    row = InstDB.Cur.fetchone ()
    if not row:
      print "*** Error: patch with Id %d not found!" % id
      continue

    print "Deleting patch '%s' with Id %d" % (row["FileName"], id)

    Util.delete_patch (id)


# Main section

if len (sys.argv) < 3: usage ()


if sys.argv[1] == "extract": do_extract (sys.argv[2:])
elif sys.argv[1] == "import": do_import (sys.argv[2:])
elif sys.argv[1] == "activate": do_activate (sys.argv[2:])
elif sys.argv[1] == "delete": do_delete (sys.argv[2:])
else: usage ()
