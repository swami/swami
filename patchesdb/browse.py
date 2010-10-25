#!/usr/bin/python
#
# browse.py - Browse instruments by category.
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
import InstDB

# Image sizes, keep synchronized with actual image sizes
image_sizes = {
  "Brass"                 : [180, 64],
  "Chromatic Percussion"  : [160, 70],
  "Collection"            : [80, 60],
  "Effects"               : [84, 49],
  "Ensemble"              : [140, 100],
  "Ethnic"                : [200, 160],
  "Guitar"                : [70, 180],
  "Miscellaneous"         : [140, 91],
  "Organ"                 : [180, 150],
  "Percussion"            : [210, 140],
  "Piano"                 : [160, 160],
  "Strings"               : [200, 91],
  "Synth"                 : [156, 80],
  "Unknown"               : [80, 75],
  "Wind"                  : [140, 90]
}

def action (req, fields):
    req.write (InstDB.header ("Browse by Category"))

    # Get count of active patch files for primary category
    InstDB.Cur.execute ("SELECT CategoryId, COUNT(*) AS UseCount FROM PatchInfo"
                        + " WHERE State='Active' GROUP BY CategoryId")
    UseCounts = {}
    for row in InstDB.Cur.fetchall ():
        UseCounts[row["CategoryId"]] = row["UseCount"]

    # Get count of active patch files for secondary category
    InstDB.Cur.execute ("SELECT CategoryId2, COUNT(*) AS UseCount"
                        " FROM PatchInfo WHERE CategoryId2 != 0"
                        " && State='Active' GROUP BY CategoryId2")
    for row in InstDB.Cur.fetchall ():
        oldcount = UseCounts.get (row["CategoryId2"], 0)
        UseCounts[row["CategoryId2"]] = oldcount + row["UseCount"]

    # Get count of active patch files for 3rd category
    InstDB.Cur.execute ("SELECT CategoryId3, COUNT(*) AS UseCount"
                        " FROM PatchInfo WHERE CategoryId3 != 0"
                        " && State='Active' GROUP BY CategoryId3")
    for row in InstDB.Cur.fetchall ():
        oldcount = UseCounts.get (row["CategoryId3"], 0)
        UseCounts[row["CategoryId3"]] = oldcount + row["UseCount"]


    # Select all categories (including sub categories)
    InstDB.Cur.execute ("SELECT CategoryId, CategoryName FROM Category"
                        " ORDER BY CategoryName")

    req.write ("<table border=0 cellpadding=4 cellspacing=0>\n")

    # Loop over categories, displaying only master categories and summing
    # sub category use counts
    count = 0
    dispcount = 0
    rows = InstDB.Cur.fetchall ()
    for i in range (0, len (rows)):
        row = rows[i]

        # Next row or None if last
        if i + 1 < len (rows): nextRow = rows[i+1]
        else: nextRow = None

        # Store ID and name of master category
        if string.find (row["CategoryName"], ':') == -1:
            id = row["CategoryId"]
            name = row["CategoryName"]

        count += UseCounts.get (row["CategoryId"], 0)

        # Next row is a sub category? - yes: still processing a master category
        if nextRow and string.find (nextRow["CategoryName"], ':') != -1:
            continue

        if dispcount % 4 == 0:
            if dispcount != 0: req.write ("</tr>\n")
            req.write ("<tr>\n")

        (width, height) = image_sizes.get (name, [100, 100])

        req.write ("""\
<td align="center">
<a href="patches.py?Action=list&amp;Category=%s">
<img src="images/category/%s.png" alt="Category Image" border=0 width=%d height=%d><br>
<b>%s</b> (%d)
</a>
</td>
""" % (id, name, width, height, name, count))

        count = 0
        dispcount += 1


    if dispcount % 4 != 1: req.write ("</tr>\n")

    req.write ("</table>")

    req.write (InstDB.footer ())
