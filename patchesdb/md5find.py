#!/usr/bin/python
#
# md5find.py - Find files by MD5 signatures
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
import re
import cgi
import string
import MySQLdb

from PatchItem import PatchHelper
import InstDB


def action (req, fields):
    sums = fields.get ("Md5Sums", None)

    req.write (InstDB.header ("MD5 Find"))

    # Form submitted?
    if sums:
        table = InstDB.tabular (["MD5", "Label", "FileName"])
        table.tableattr = 'width="100%"'

        reObj = re.compile ("[0-9a-fA-F]{32}")

        for line in sums.split ('\n'):
            if not reObj.match (line): continue

            md5 = line[:32]
            label = line[32:].strip ()

            InstDB.Cur.execute ("SELECT PatchId, FileName FROM Files WHERE Md5=%s", md5)
            result = InstDB.Cur.fetchone ()

            if result:
                filelink = '<a href="patches.py?Action=item&amp;ItemId=%d">%s</a>' \
                    % (result["PatchId"], result["FileName"])
            else: filelink = "Not found"

            req.write (table.addrow ([md5, label, filelink]))

        req.write (table.end ())
        req.write (InstDB.footer ())
        return

    box = InstDB.html_box ()
    req.write (box.start ("MD5 Find"))

    req.write ("""
<form action="" method=post>
<table border=0 cellpadding=8>
<tr>
<td>
Enter one MD5 signature per line optionally followed by white space and a
file name (or any other text).  The MD5 signatures will be searched for in the
database.  The correct text output can be obtained from programs like <b>md5sum</b> and
others (see Wikipedia <a href="http://en.wikipedia.org/wiki/Md5sum">Md5sum</a>).<br>
Example: <i>752091450e308127a4d70c7623dd4e37  Fluid.sf2</i><br>
</td>
</tr>
<tr>
<td>
<textarea name=Md5Sums rows=16 cols=80>
</textarea>
</td>
</tr>
<tr>
<td>
<input type=submit name=Submit value=Submit>
</td>
</tr>
</table>
</form>
""")

    req.write (box.end ())
    req.write (InstDB.footer ())
