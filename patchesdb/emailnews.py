#!/usr/bin/python
#
# emailnews.py - Mass email a message to users who have EmailNews enabled
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

def action (req, fields):

    if not InstDB.User or "Admin" not in InstDB.User["Flags"]:
        req.write (InstDB.header ("Admin only"))
        InstDB.error (req, "Administrator privileges required for this function")
        req.write (InstDB.footer ())
        return

    req.write (InstDB.header ("Email news"))

    subject = fields.get ("Subject", None)
    msg = fields.get ("Message", None)

    # Message submitted?
    if subject and msg:
        message = "Subject: " + subject + "\r\n\r\n" + msg

        query = "SELECT UserId FROM Users WHERE !FIND_IN_SET('NoLogin', Flags)"

        allUsers = fields.get ("AllUsers", None)

        if not allUsers:
            query += " && FIND_IN_SET('EmailNews', UserFlags)"

        InstDB.Cur.execute (query)

        for row in InstDB.Cur.fetchall ():
            # Queue email task
            InstDB.Cur.execute ("INSERT INTO Queue (Type, Status, UserId, Content)"
                                " VALUES ('Email', 'Queued', %s, %s)",
                                (row["UserId"], message))

        if allUsers: allstr = "ALL"
        else: allstr = "EmailNews"

        req.write ('<font class="title">Sent message to %s users.</font>\n'
                   % allstr)
        req.write (InstDB.footer ())
        return

    box = InstDB.html_box ()
    req.write (box.start ("Email news"))

    req.write ("""
<form action="patches.py?Action=emailnews" method=post>
<table border=0 cellpadding=8>
<tr>
<td><b>Subject:</b></td>
<td><input type=text name=Subject size=64></td>
</tr>
<tr>
<td><b>All users:</b></td>
<td><input type=checkbox name=AllUsers> Email <b>ALL</b> users?</td>
</tr>
<tr>
<td colspan=2>
<textarea name=Message rows=16 cols=80>
</textarea>
</td>
</tr>
<tr>
<td colspan=2>
<input type=submit name=Submit value=Submit>
</td>
</tr>
</table>
</form>
""")

    req.write (box.end ())
    req.write (InstDB.footer ())
