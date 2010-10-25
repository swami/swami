#!/usr/bin/python
#
# SqlHelpers.py - Helper classes for creating SQL queries.
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

import string
import MySQLdb

class Select:
    "SQL Select query object"

    def __init__ (self):
        self.fields = []                # List of fields or values to select
        self.tables = []                # List of tables to select from
        self.where = []                 # List of WHERE clauses (ANDed)
        self.orderby = ""               # Order by clause (ex. "last, first")
        self.limit = ""                 # Limit clause (ex. "100, 50")
        self.calc_found_rows = False    # Set to TRUE to add SQL_CALC_FOUND_ROWS

    def query (self):
        if not (self.fields and self.tables): return None

        qstr = "SELECT "

        if self.calc_found_rows:
            qstr += "SQL_CALC_FOUND_ROWS "

        qstr += string.join (self.fields, ",") + " FROM " \
                + string.join (self.tables, ",")
        if self.where: qstr += " WHERE " + string.join (self.where, " && ")
        if self.orderby: qstr += " ORDER BY " + self.orderby
        if self.limit: qstr += " LIMIT " + self.limit

        return qstr

def quote (val):
    return '"' + MySQLdb.escape_string (str (val)) + '"'

class Insert:
    "SQL Insert class object"

    def __init__ (self):
        self.table = ""   # Table to insert into
        self.set = {}     # Set of Name/Value pairs to insert (escaped)
        self.setnesc = {} # Set of Name/Value pairs to insert (not escaped)
        self.select = ""  # Instead of using values in "set" use select query

    def query (self):
        if not self.table or not (self.set or self.select): return None

        qstr = "INSERT INTO " + self.table
        if self.set or self.setnoesc:
            qstr += " (" + string.join (self.setnesc.keys()
                + self.set.keys(), ",") + ") "

        if self.select: qstr += self.select
        else:
            qstr += "VALUES (" + string.join (self.setnesc.values ()
                + map (quote, self.set.values ()), ",") + ")"

        return qstr

class Update:
    "SQL Update query object"

    def __init__ (self):
        self.table = ""             # Table to update
        self.set = {}               # Hash of Field/Values to set (escaped)
        self.setnesc = {}           # Hash of Field/Values to set (not escaped)
        self.where = []             # List of WHERE clauses (ANDed)

    def query (self):
        if not self.table or not (self.set or self.setnesc): return None

        # Remove any duplicate fields (set overrides setnesc to be safe)
        if self.set and self.setnesc:
            for field in self.set.keys ():
                if self.setnesc.get (field, None):
                    del self.setnesc[field]
                    raise ValueError, "Duplicate field '%s' in SQL update (set/setnesc)" % field

        qstr = "UPDATE " + self.table + " SET "
        fields1 = [k + "=" + v for (k, v) in self.setnesc.iteritems ()]
        fields2 = [k + '="' + MySQLdb.escape_string (str (v)) + '"'
                   for (k, v) in self.set.iteritems ()]
        qstr += string.join (fields1 + fields2, ",")
        if self.where: qstr += " WHERE " + string.join (self.where, " && ")

        return qstr
