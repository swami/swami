#!/usr/bin/python
#
# edit.py - Edit patch information.
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

from urllib import urlencode
import string

from PatchItem import *
import InstDB

def action (req, fields):
    id = int (fields["ItemId"])

    req.write (InstDB.header ("Edit patch file details"))

    InstDB.Cur.execute ("SELECT UserId FROM PatchInfo WHERE PatchId=%s", id)
    row = InstDB.Cur.fetchone ()
    if not row:
        InstDB.error (req, "Patch file with ID '%d' not found" % id)
        req.write (InstDB.footer ())
        return

    # Do some security checks
    if not InstDB.User:
        InstDB.error (req, "Must login to edit patch information")
        req.write (InstDB.footer ())
        return

    if not "Admin" in InstDB.User["Flags"] \
       and row["UserId"] != InstDB.User["UserId"]:
        InstDB.error (req, "You don't have permission to edit that info")
        req.write (InstDB.footer ())
        return

    edit_patch_item (id, fields, req)

    req.write (InstDB.footer ())


def edit_patch_item (id, fields, req):
    itemCells = PatchHelper.edit_cell_format
    itemFields = InstDB.cells_to_fields (itemCells)

    InstDB.Cur.execute ("SELECT PropId, PropName FROM Props ORDER BY PropName")
    propnames = InstDB.Cur.fetchall ()

    # Was Save button clicked? - Save field values
    if fields.get ("Save", None):
        update = PatchHelper.sql_update (itemFields, fields)
        update.where = ("PatchId=%d" % id,)
        InstDB.Cur.execute (update.query ())

        props_update (id, fields)

    sel = PatchHelper.sql_select (itemFields)
    sel.where.insert (0, "PatchInfo.PatchId=%d" % id)
    InstDB.Cur.execute (sel.query ())

    row = InstDB.Cur.fetchone ()
    if not row:         # Shouldn't happen, thats why we raise an exception
        raise LookupError, "Patch file with ID '%d' not found" % id

    sel = props_select (id)
    InstDB.Cur.execute (sel.query ())
    props = InstDB.Cur.fetchall ()

    # Get count of Instruments in this patch
    InstDB.Cur.execute ("SELECT COUNT(*) AS Count FROM InstInfo"
                        " WHERE PatchId=%s" % id)
    instcount = InstDB.Cur.fetchone ()["Count"]

    # Get count of samples in this patch
    InstDB.Cur.execute ("SELECT COUNT(*) AS Count FROM SampleInfo"
                        " WHERE PatchId=%s" % id)
    samplecount = InstDB.Cur.fetchone ()["Count"]

    tab = PatchHelper.form_cell_vals (itemCells, row)

    if instcount > 0:
        trow = ('<a href="patches.py?Action=list&amp;Type=Inst&amp;PatchId=%s'
                '&amp;OrderBy=Bank">Instruments</a>' % id, instcount)
    else: trow = ("Instruments", "None")

    if samplecount > 0:
        trow += ('<a href="patches.py?Action=list&amp;Type=Sample&amp;PatchId=%s'
                 '&amp;OrderBy=SampleNameLink">Samples</a>' % id, samplecount)
    else: trow += ("Samples", "None")

    tab += ((trow,))
    tab += props_form_cells (props)

    req.write ('<form action="" method=POST>\n')

    # Display patch item edit form
    box = InstDB.html_box ()

    box.titlehtml = '\n&nbsp;&nbsp;&nbsp;<a href="patches.py?Action=item&amp;ItemId=%d">' \
                '<img src="images/view.png" alt="View Mode" width=15 height=15 border=0>' \
                '&nbsp;<font class="boxtitle">View</font></a>\n' % id   # Show edit icon

    req.write (box.start ("Patch File"))

    split = InstDB.tabsplits (tab)
    req.write (split.render ())

    req.write ("<center><input type=submit name=Save value=\"Save\">"
               "</center>\n")

    req.write (box.end ())

    req.write ("</form>\n<p>\n")
