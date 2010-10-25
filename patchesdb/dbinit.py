#!/usr/bin/python
#
# dbinit.py - One time database initialization.
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

import os
import string

import InstDB

def action (req, fields):
  req.write (InstDB.header ("Initialize database"))

  if InstDB.Cur.execute ("SHOW tables") == 0:
    for fname in os.listdir (InstDB.BasePath + os.sep + "sql"):
      if fname[-4:] != ".sql": continue

      req.write ("Executing SQL in '%s'<br>\n" % fname)

      f = open (InstDB.BasePath + os.sep + "sql" + os.sep + fname, "r")
      sql = f.read ()

      # Split text file into individual queries
      for query in string.split (sql, ';')[:-1]:
        InstDB.Cur.execute (query)

    # Import categories file
    f = open (InstDB.BasePath + os.sep + "sql" + os.sep + "categories.txt", "r")
    for line in f.readlines():
      fields = string.split (line, "\t")
      InstDB.Cur.execute ("INSERT INTO Category (CategoryName, CategoryDescr)"
                          " VALUES (%s, %s)", (fields[0], fields[1]))

    # Import licenses file
    f = open (InstDB.BasePath + os.sep + "sql" + os.sep + "licenses.txt", "r")
    for line in f.readlines():
      fields = string.split (line, "\t")
      InstDB.Cur.execute ("INSERT INTO Licenses (LicenseName, LicenseDescr,"
                          " LicenseURL) VALUES (%s, %s, %s)",
                          (fields[0], fields[1], fields[2]))
  else:
    req.write ("Database already contains tables, nothing done")

  req.write (InstDB.footer ())
