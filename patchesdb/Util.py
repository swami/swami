#!/usr/bin/python
#
# Util.py - PatchesDB utility functions
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

import InstDB
import string

def delete_patch(patchId):
    """Delete an instrument from the database"""

    InstDB.Cur.execute ("LOCK TABLES FileMarks WRITE, Files WRITE,"
                        " InstInfo WRITE, ItemProps WRITE, Items WRITE,"
                        " PatchInfo WRITE, Ratings WRITE, SampleInfo WRITE")

    InstDB.Cur.execute ("DELETE FROM Files WHERE PatchId=%s", patchId)
    InstDB.Cur.execute ("DELETE FROM FileMarks WHERE PatchId=%s", patchId)
    InstDB.Cur.execute ("DELETE FROM Ratings WHERE PatchId=%s", patchId)

    InstDB.Cur.execute ("SELECT SampleId FROM SampleInfo WHERE PatchId=%s",
                        patchId)
    itemIds = []
    for row in InstDB.Cur.fetchall ():
        itemIds.append (row["SampleId"])

    InstDB.Cur.execute ("DELETE FROM SampleInfo WHERE PatchId=%s", patchId)

    InstDB.Cur.execute ("SELECT InstId FROM InstInfo WHERE PatchId=%s", patchId)
    for row in InstDB.Cur.fetchall ():
        itemIds.append (row["InstId"])

    InstDB.Cur.execute ("DELETE FROM InstInfo WHERE PatchId=%s", patchId)
    InstDB.Cur.execute ("DELETE FROM PatchInfo WHERE PatchId=%s", patchId)
    InstDB.Cur.execute ("DELETE FROM ItemProps WHERE ItemId=%s", patchId)

    # Split delete into blocks of 512 to prevent MySQL query buffer overflow
    idlen = len (itemIds)
    for i in range (0, idlen, 512):
        end = i + 512
        if end > idlen: end = idlen

        InstDB.Cur.execute ("DELETE FROM Items WHERE ItemId IN (%s)" %
                            string.join (map (lambda x: str (x),
                                              itemIds[i:end]), ','))

    InstDB.Cur.execute ("UNLOCK TABLES")
