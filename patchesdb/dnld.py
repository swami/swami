#!/usr/bin/python
#
# dnld.py - Instrument download.
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
from PatchItem import *
from mod_python import apache

def action (req, fields):
    id = int (fields.get ("PatchId", 0))

    filetype = fields.get ("Type", None)
    if filetype != "zip": filetype = "CRAM"

    itemFields = ("FileName", "PatchType", "ZipSize", "CramSize")
    sel = PatchHelper.sql_select (itemFields)

    sel.fields.append ("DATEDIFF(NOW(), PatchInfo.DateImported) AS ImportDays")   # Days since imported
    sel.where.insert (0, "PatchInfo.State = 'Active'")
    sel.where.insert (0, "PatchInfo.PatchId=%d" % id)

    InstDB.Cur.execute (sel.query ())

    PatchRow = InstDB.Cur.fetchone ()
    if not PatchRow:
        req.write (InstDB.header ("Download Error"))
        InstDB.error (req, "Instrument file with ID '%d' not found" % id)
        req.write (InstDB.footer ())
        return

    # For now, we just select a random mirror, unless imported less than 2 days ago, then we use default mirror
    if (PatchRow["ImportDays"] > 2):
        InstDB.Cur.execute ("SELECT MirrorId, BaseURL FROM Mirrors"
                            " WHERE FIND_IN_SET('Active', Flags) && FIND_IN_SET(%s, Flags)"
                            " ORDER BY RAND() LIMIT 1", filetype)
    else:  # Just use first active mirror with file type available (usually Mirror 1, unless its inactive)
      InstDB.Cur.execute ("SELECT MirrorId, BaseURL FROM Mirrors WHERE"
                          " FIND_IN_SET('Active', Flags) && FIND_IN_SET(%s, Flags)"
                          " ORDER BY MirrorId LIMIT 1", filetype)

    mirror = InstDB.Cur.fetchone ()
    if not mirror:
        req.write (InstDB.header ("Download Error"))
        InstDB.error (req, "No active mirror found for requested instrument file")
        req.write (InstDB.footer ())
        return

    # Update PatchInfo file download count for file type and set some vars
    if filetype == "zip":
        filesize = PatchRow["ZipSize"]
        fileext = "zip"
        InstDB.Cur.execute ("UPDATE PatchInfo SET ZipClicks=ZipClicks+1"
                            " WHERE PatchId=%s", id)
    else:
        filesize = PatchRow["CramSize"]
        fileext = "cram"
        InstDB.Cur.execute ("UPDATE PatchInfo SET CramClicks=CramClicks+1"
                            " WHERE PatchId=%s", id)

    # Update Mirror download count and download size counters
    InstDB.Cur.execute ("UPDATE Mirrors SET DownloadClicks=DownloadClicks+1,"
                        "DownloadSize=DownloadSize+%s,DownloadTotal=DownloadTotal+%s"
                        " WHERE MirrorId=%s", (filesize, filesize, mirror["MirrorId"]))

    # If user logged in, mark file as downloaded
    if InstDB.User:
        InstDB.Cur.execute ("SELECT COUNT(*) AS count FROM FileMarks"
                            " WHERE UserId=%s && PatchId=%s",
                            (InstDB.User['UserId'], id))
        if InstDB.Cur.fetchone()["count"] == 0:
            InstDB.Cur.execute ("INSERT FileMarks (UserId, PatchId, MarkFlags)"
                                " VALUES(%s,%s,'Downloaded')",
                                (InstDB.User['UserId'], id))
        else:
            InstDB.Cur.execute ("UPDATE FileMarks SET MarkFlags=CONCAT(MarkFlags, \",Downloaded\"),"
                                "MarkTime=Null WHERE UserId=%s && PatchId=%s",
                                (InstDB.User['UserId'], id))

    # Redirect to file to download
    if req.proto_num >= 1001:
        req.status = apache.HTTP_TEMPORARY_REDIRECT
    else: req.status = apache.HTTP_MOVED_TEMPORARILY

    # FIXME - Does redirect Location: URL need to be escaped???
    req.headers_out["Location"] = "%s/%s.%s.%s" \
      % (mirror["BaseURL"], PatchRow["FileName"], PatchRow["PatchType"], fileext)
    req.send_http_header()
