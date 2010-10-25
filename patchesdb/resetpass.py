#!/usr/bin/python
#
# resetpass.py - Password reset form.
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

import InstDB
import confirm

def action (req, fields):
    req.write (InstDB.header ("Reset password"))

    login = fields.get ("Login", None)
    email = fields.get ("Email", None)

    if login or email:
        if login: InstDB.Cur.execute ("SELECT UserId, Login, UserEmail FROM Users"
                                      " WHERE Login=%s", login)
        else: InstDB.Cur.execute ("SELECT UserId, Login, UserEmail FROM Users"
                                  " WHERE UserEmail=%s", email)

        row = InstDB.Cur.fetchone ()
        if not row:
            InstDB.error (req, "Sorry, no account matches the given input.")
            req.write (InstDB.footer ())
            return

        userid = row["UserId"]
        emailaddr = row["UserEmail"]
        login = row["Login"]

        InstDB.Cur.execute ("SELECT COUNT(*) AS count FROM Confirm"
                            " WHERE UserId=%s && ConfirmAction='ResetPass'",
                            userid)

        if InstDB.Cur.fetchone ()["count"]:
            InstDB.error (req, "Password reset confirmation already sent.")
            req.write ('<p>\nIf you do not receive it soon, feel free to'
                       ' email the <a href="mailto:sounds-admin@resonance.org">admin</a>'
                       ', or wait a day for this confirmation to expire.')
            req.write (InstDB.footer ())
            return

        confirm.new ('ResetPass', userid, login)

        req.write ('<font class="title">An email will be sent soon'
                   ' to the email address you registered with, containing'
                   ' directions on how to reset your password.</font>\n')

        req.write (InstDB.footer ())
        return

    box = InstDB.html_box ()
    req.write (box.start ("Reset password"))

    req.write ("""
<form action="patches.py?Action=resetpass" method=post>
<table border=0 cellpadding=8>
<tr>
<td align="center" bgcolor="#4070b0">
<b>Login</b><input type=text name=Login size=16><br>
or<br>
<b>Email</b><input type=text name=Email size=16>
<p>
<input type=submit value="Reset password">
<p>
</td>
<td>
Enter your account login <b>OR</b> email address. A confirmation email
will be sent to your registered email address with directions
for resetting your password.
</td>
</tr>
</table>
</form>
""")

    req.write (box.end ())
    req.write (InstDB.footer ())
