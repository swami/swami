#!/usr/bin/python
#
# profile.py - Profile editing.
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
from PatchItem import *
import InstDB

def valify (value):
    if not value: return ""
    return " value=\"%s\"" % value

def action (req, fields):
    req.write (InstDB.header ("Edit profile"))

    if not InstDB.AssertLogin(req):
        req.write (InstDB.footer ())
        return

    itemCells = UsersHelper.edit_cell_format
    itemFields = InstDB.cells_to_fields (itemCells)

    errmsg = None

    # Was Save button clicked? - Update field values
    if fields.get ("Save", None):
        data = fields

        password = fields.get ("Password", "")
        verify = fields.get ("PassVerify", "")
        email = fields.get ("UserEmail", "")

        if len (password) > 0 and len (password) < 5:
            errmsg = "Passwords must be 5-32 characters, leave blank for no change"

        if not errmsg and password != verify:
            errmsg = "Password and Verify don't match"

        if not errmsg:
            n = string.find (email, "@")
            n2 = string.find (email, " ")
            if n <= 0 or n == len (email) - 1 or n2 >= 0:
                errmsg = "A valid email address is required"

        if not errmsg:
            update = UsersHelper.sql_update (itemFields, fields)

            if password: update.setnesc["Password"] = 'SHA1("%s")' \
                % MySQLdb.escape_string (password + InstDB.FuKraXor)

            update.where = ("UserId=%d" % InstDB.User["UserId"],)

            # Use login if UserName not set
            if not fields.get ("UserName", None):
                update.setnesc["UserName"] = "Login"
                if update.set.get ("UserName"): del update.set["UserName"]

            InstDB.Cur.execute (update.query ())
            req.write ("<font size=+1>User profile updated</font>\n<p>\n")
        else:
            req.write ('<font class="error">%s</font><p>\n' % errmsg)
    else:       # First page load?
        sel = UsersHelper.sql_select (itemFields)
        sel.where.insert (0, "UserId=%d" % InstDB.User["UserId"])
        InstDB.Cur.execute (sel.query ())
        data = InstDB.Cur.fetchone ()

    tab = (("Password", "<input type=password name=Password>"),
           ("Verify", "<input type=password name=PassVerify>"))
    tab += tuple (UsersHelper.form_cell_vals (itemCells, data))
    tab += (("<input type=submit name=Save value=\"Save\">\n",),)

    # Display form
    box = InstDB.html_box ()
    req.write (box.start ("User Profile"))
    req.write ("<table border=0 cellpadding=8>\n<tr>\n<td valign=top bgcolor=#408ee6>\n");
    req.write ("<form action=\"patches.py?Action=profile\" method=POST>\n")

    split = InstDB.tabsplits (tab)
    req.write (split.render ())

    req.write ("</form>\n");
    req.write ("</td>\n<td valign=top>\n")

    req.write ("<vr>\n</td>\n<td valign=top>\n")

    req.write (InstDB.IncFile ("profile.inc"))

    req.write ("</td>\n</tr>\n</table>\n")
    req.write (box.end ())

    req.write (InstDB.footer ())
