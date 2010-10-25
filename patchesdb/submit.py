#!/usr/bin/python
#
# submit.py - Submit patch files.
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

import cgi
import os
import time
import re

import SqlHelpers
from PatchItem import *
import InstDB

# Display patch edit form
def display_patch (req, row, fields):
    itemCells = InstDB.delete_cells (PatchHelper.edit_cell_format, "FileName")
    prefix = "P%d_" % row["PatchId"]    # Prefix field names with patch ID

    tab = [["Activate", "<input type=checkbox name=%sActivate>" % prefix],
           ["File Name", '<input type=text name=%sFileName value="%s" size=48>'
            '.%s' % (prefix, row["FileName"], row["PatchType"])]]

    tab += PatchHelper.form_cell_vals (itemCells, row, prefix)

    patchid = row["PatchId"]

    # Get patch property values
    sel = props_select (patchid)
    InstDB.Cur.execute (sel.query ())
    props = InstDB.Cur.fetchall ()

    # Get count of Instruments in this patch
    InstDB.Cur.execute ("SELECT COUNT(*) AS Count FROM InstInfo"
                        " WHERE PatchId=%s", patchid)
    instcount = InstDB.Cur.fetchone ()["Count"]

    # Get count of samples in this patch
    InstDB.Cur.execute ("SELECT COUNT(*) AS Count FROM SampleInfo"
                        " WHERE PatchId=%s", patchid)
    samplecount = InstDB.Cur.fetchone ()["Count"]

    if instcount > 0:
        trow = ('<a href="patches.py?Action=list&amp;Type=Inst&amp;PatchId=%s'
                '&amp;OrderBy=Bank">Instruments</a>' % patchid, instcount)
    else: trow = ("Instruments", "None")

    if samplecount > 0:
        trow += ('<a href="patches.py?Action=list&amp;Type=Sample&amp;PatchId=%s'
                 '&amp;OrderBy=Name">Samples</a>' % patchid, samplecount)
    else: trow += ("Samples", "None")

    tab += ((trow,))

    tab += props_form_cells (props, prefix)

    # Get list of extracted files
    patchFilePath = InstDB.ActivatePath + os.sep + str(row["PatchId"]) + "-files"
    file_list = os.listdir (patchFilePath)
    file_list.sort ()

    table = InstDB.tabular (("File name", "Size"))
    table.tableattr = 'width="100%"'
    table.ttlrowattr = 'bgcolor="#110044"'

    tableStr = ""
    for fname in file_list:
        if fields: newname = fields.get ("File_%s" % fname, None)
        else: newname = None

        if newname and newname != fname:    # File rename requested?
            (evil, newname) = os.path.split (newname) # Make sure no path of evil
            try:
                os.rename (patchFilePath + os.sep + fname,
                           patchFilePath + os.sep + newname)
                fname = newname
            except:
                InstDB.error (req, "Failed to rename file '%s' to '%s'!"
                              % (fname, newname))

        namefield = '<input type=text name="%sFile_%s" value="%s" size=60>' \
            % (prefix, cgi.escape (fname), cgi.escape (fname))
        stats = os.stat (patchFilePath + os.sep + fname)
        tableStr += table.addrow ((namefield,
                                  InstDB.pretty_size (stats.st_size)))
    tableStr += table.end ()

    tab += (("<center><b>Archive files</b></center>\n",),)
    tab += ((tableStr,),)

    split = InstDB.tabsplits (tab)
    req.write (split.render ())

    req.write ("<p>\n")


def action (req, fields):
    req.write (InstDB.header ("Patch submission"))

    if not InstDB.AssertLogin (req):
        req.write (InstDB.footer ())
        return

    req.write ("""
Upload files to <a href="ftp://sounds.resonance.org/incoming/">ftp://sounds.resonance.org/incoming/</a>.
It is recommended that you read the <a href="patches.py?Action=help&amp;Topic=submit">Content Submission Help</a>.
<p>
""")

    # Check if any incoming files have been selected
    Files = fields.getlist ("Files")
    for fname in Files:
        (evil, fname) = os.path.split (fname) # Make sure no path of evil

        if not os.path.isfile (InstDB.IncomingPath + os.sep + fname):
            InstDB.error (req, "File not found '%s'" % cgi.escape (fname))
        else:
            # Create new import queue task (don't activate yet)
            InstDB.Cur.execute ("INSERT INTO Queue"
                                " (Type, Status, FileName, UserId)"
                                " VALUES ('Import', 'Queued', %s, %s)",
                                (fname, InstDB.User["UserId"]))

    # Get list of files in incoming directory
    file_list = os.listdir (InstDB.IncomingPath)
    file_list.sort ()

    box = InstDB.html_box ()
    box.tableattr = 'width="100%"'
    req.write (box.start ("Incoming files"))

    req.write ("<form action=\"patches.py?Action=submit\" method=POST>\n")

    if file_list:
        table = InstDB.tabular (("Import", "File", "Size", "Date"))
        table.tableattr = 'width="100%"'

        for file in file_list:
            check = "<input type=checkbox name=Files value=\"%s\">" \
                    % cgi.escape (file)
            stats = os.stat (InstDB.IncomingPath + "/" + file)
            req.write (table.addrow ((check, cgi.escape (file),
                                      InstDB.pretty_size (stats.st_size),
                                      time.ctime (stats.st_mtime))))
        req.write (table.end ())

    else: req.write ("<b>No files in incoming directory</b><br>\n")

    req.write (box.end())

    req.write ("<center><input type=submit name=FileSelect"
               " value=\"Import and/or Refresh\"></center>\n")
    req.write ("</form>\n")    

    req.write ("<p><p>\n")


    # Get queued tasks

    box = InstDB.html_box ()
    box.tableattr = 'width="100%"'
    req.write (box.start ("Your queued tasks"))

    sel = SqlHelpers.Select ()
    sel.fields = ("QueueId", "Type", "Status", "UserId", "FileName", "Date")
    sel.tables = ("Queue",)
    sel.where = ("Type in ('Import','Activate')",)
    sel.orderby = "QueueId"

    InstDB.Cur.execute (sel.query ())

    table = InstDB.tabular (("Position", "File Name", "Action", "Status",
                            "Start Time"))
    table.tableattr = 'width="100%"'

    index = 0
    any = False
    for row in InstDB.Cur.fetchall ():
        if row["UserId"] == InstDB.User["UserId"]:
            any = True

            if row["Status"] != "Error":
                ndxval = index
                status = row["Status"]
            else:
                ndxval = "N/A"
                status = '<font class="Error">' + row["Status"] + '</font>'

            req.write (table.addrow ((ndxval, cgi.escape (row["FileName"]),
                                      row["Type"], status, row["Date"])))
        if row["Status"] != "Error":
            index += 1

    if any: req.write (table.end ())
    else: req.write ("<b>No queued tasks</b><br>\n")

    req.write (box.end())
    req.write ("<p><p>\n")


    # Any activation form data submitted?  Group fields by patch ID.
    patchFields = {}
    if fields.get ("Activate", None):
        for field in fields.list:
            # Fields have "P<PatchId>_" prefix
            match = re.match ("P([0-9]{1,10})_(.+)", field.name)
            if match:
                patchId = match.group (1)
                if not patchFields.get (patchId, None):
                    patchFields[patchId] = {}

                patchFields[patchId][match.group(2)] = fields[field.name]

    # Files pending activation

    box = InstDB.html_box ()
    box.tableattr = 'width="100%"'
    req.write (box.start ("Files pending activation"))

    InstDB.Cur.execute ("SELECT PatchId, PatchType FROM PatchInfo"
                        " WHERE UserId=%s && State='Imported'",
                        InstDB.User["UserId"])
    pendRows = InstDB.Cur.fetchall ()

    if pendRows:
        activateCells = PatchHelper.edit_cell_format
        activateFields = InstDB.cells_to_fields (activateCells)
    displayedOne = False

    for pendRow in pendRows:
        pFields = patchFields.get (str (pendRow["PatchId"]), None)
        patchId = int (pendRow["PatchId"])
        activate = pFields and pFields.get ("Activate", None)
        errorMsg = ""

        if pFields:         # Form data submitted for this patch?
            pFields["FileName"] = pFields["FileName"].strip ()
            if not pFields["FileName"]:
                errorMsg = "File name is required"

            # Check if filename already used
            if not errorMsg:
                InstDB.Cur.execute ("SELECT COUNT(*) AS count FROM PatchInfo"
                                    " WHERE PatchId != %s && FileName=%s"
                                    " && PatchType=%s",
                                    (patchId, pFields["FileName"],
                                     pendRow["PatchType"]))
                if int (InstDB.Cur.fetchone()["count"]) > 0:
                    errorMsg = "File name already in use"

        if pFields:
            update = PatchHelper.sql_update (activateFields, pFields)
            update.set["FileName"] = pFields["FileName"]
            update.where = ("PatchInfo.PatchId=%d" % patchId,)
            if activate and not errorMsg: update.set["State"] = "Activating"

            InstDB.Cur.execute (update.query ())
            props_update (patchId, pFields)     # Update extra properties

            # Queue activation task
            if activate and not errorMsg:
                InstDB.Cur.execute ("INSERT INTO Queue"
                                    " (Type, Status, UserId, ItemId, FileName)"
                                    " VALUES ('Activate', 'Queued', %s, %s, %s)",
                                    (InstDB.User["UserId"], patchId,
                                     "%s.%s" % (pFields["FileName"],
                                     pendRow["PatchType"])))

        if (not activate or errorMsg) and not displayedOne:
            req.write ("<form action=\"patches.py?Action=submit\" method=POST>\n")
            displayedOne = True

        if errorMsg:
            req.write ('<font class="error">%s</font><br>\n'
                       % cgi.escape (errorMsg))

        if not activate or errorMsg:
            sel = PatchHelper.sql_select (activateFields)
            sel.fields.insert (0, "PatchInfo.PatchId")
            sel.where.insert (0, "PatchInfo.PatchId=%d" % patchId)
            InstDB.Cur.execute (sel.query ())
            row = InstDB.Cur.fetchone ()
            display_patch (req, row, pFields)

    if displayedOne:
        req.write ("<input type=submit name=Activate"
                   " value=\"Update\">\n")
        req.write ("</form>\n")
    else: req.write ("<b>No files pending activation</b><br>\n")

    req.write (box.end())
    req.write (InstDB.footer ())
