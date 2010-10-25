#!/usr/bin/python
#
# index.py - Main index page.
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

MaxNewsItems = 25

def action (req, fields):
    global MaxNewsItems

    req.write (InstDB.header ("Menu"))

    box = InstDB.html_box ()
    req.write (box.start ("About"))
    req.write (InstDB.IncFile ("about.inc"))
    req.write (box.end ())

    req.write ("<p>\n")

    InstDB.Cur.execute ("""\
SELECT DATE_FORMAT(N.Date,'%%a, %%Y-%%m-%%d %%H:%%i:%%s') AS Date,
N.UserId, U.UserName, N.Title, N.Content FROM News AS N, Users AS U
WHERE N.UserId=U.UserId ORDER BY N.Date DESC LIMIT %s""", MaxNewsItems)

    for entry in InstDB.Cur.fetchall ():
        req.write ("""\
<p>
<font class=newstitle>%s</font><br>
<table width="100%%" border=0 cellpadding=4 cellspacing=0><tr class=lightbg><td>
Submitted by <a href="patches.py?Action=User&amp;UserId=%s">%s</a> on %s
<p>
%s
</td></tr></table>
""" % (entry["Title"], entry["UserId"], entry["UserName"], entry["Date"],
       entry["Content"]))

    req.write (InstDB.footer ())
