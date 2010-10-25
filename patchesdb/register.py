#!/usr/bin/python
#
# register.py - User account registration.
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
import string

import confirm
import phpbb
from PatchItem import *
import InstDB

from recaptcha.client.captcha import displayhtml as recaptcha_display
from recaptcha.client.captcha import submit as recaptcha_submit

def valify (value):
    if not value: return ""
    return " value=\"%s\"" % value

def verify_chars (str, allowed):
    for c in str:
        if not c in allowed: return False
    return True

def action (req, fields):
    req.write (InstDB.header ("Registration"))

    itemCells = UsersHelper.edit_cell_format

    errmsg = None
    recaptcha_error = None

    # Was Register button clicked? - Save field values
    if fields.get ("Register", None):
        data = fields

        login = fields.get ("Login", "")
        password = fields.get ("Password", "")
        verify = fields.get ("PassVerify", "")
        email = fields.get ("UserEmail", "")
        captcha_challenge = fields.get ("recaptcha_challenge_field", "")
        captcha_response = fields.get ("recaptcha_response_field", "")

        if not login: errmsg = "Login name is required"
        elif not verify_chars (login, InstDB.LoginAllowChars):
            errmsg = "Only these characters are allowed in login: '%s'" \
                % InstDB.LoginAllowChars
        elif login and InstDB.Cur.execute \
                 ("SELECT 1 FROM Users WHERE Login=%s", login) == 1:
            errmsg = "The login '%s' is already used" % (cgi.escape (login))

        if not errmsg and len (password) < 5:
            errmsg = "A password with 5-32 characters is required"

        if not errmsg and password != verify:
            errmsg = "Password and Verify don't match"

        if not errmsg:
            n = string.find (email, "@")
            n2 = string.find (email, " ")
            if n <= 0 or n == len (email) - 1 or n2 >= 0:
                errmsg = "A valid email address is required"

        if not errmsg and not verify_chars (email, InstDB.EmailAllowChars):
            errmsg = "Only these characters are allowed in email address: '%s'" \
                % InstDB.EmailAllowChars

        if not errmsg and InstDB.Cur.execute \
                ("SELECT Login FROM Users WHERE UserEmail=%s", email) == 1:
            existLogin = InstDB.Cur.fetchone()["Login"]
            errmsg = "Login '%s' is already registered with that email address." \
                % existLogin

        if not errmsg:
            resp = recaptcha_submit (captcha_challenge, captcha_response,
                                     InstDB.ReCaptchaPrvKey, req.connection.remote_ip)
            if not resp.is_valid:
                errmsg = "Are you human? reCAPTCHA response incorrect."
                recaptcha_error = resp.error_code

        if not errmsg:
            itemFields = InstDB.cells_to_fields (InstDB.delete_cells (itemCells, "UserName"))

            insert = UsersHelper.sql_insert (itemFields, fields)
            insert.set["Login"] = login
            insert.set["Flags"] = "Confirm"
            insert.setnesc["Password"] = 'SHA1("%s")' \
                % MySQLdb.escape_string (password + InstDB.FuKraXor)
            insert.setnesc["DateCreated"] = "NULL"
            insert.set["LastLogin"] = 0

            # Use login if UserName not set
            username = fields.get ("UserName", None)
            if not username: username = login
            insert.set["UserName"] = username

            InstDB.Cur.execute (insert.query ())
            userid = InstDB.DB.insert_id ()

            confirm.new ("Register", userid, login)

            # Add new account for phpbb forums
            if InstDB.EnableForums:
                retval = phpbb.add_user (login, password, email)

                if retval != 0:
                  raise RuntimeWarning, "Failed to add user '%s' to PHPBB forums (retval=%d)" % (login, retval)

            req.write ('<font class="title">Registration successful, an email'
                       ' will be sent shortly with directions on activating'
                       ' your account.</font>\n')
            req.write (InstDB.footer ())
            return
        else:
            InstDB.error (req, errmsg)
            req.write ("<p>\n")
    else:
        data = { "UserFlags" : ('EmailNews', 'NotifyMessage', 'NotifyRatings') }

    tab = (("Login", '<input type=text name=Login value="%s">' %
            cgi.escape (data.get ("Login", ""))),
           ("Password", "<input type=password name=Password>"),
           ("Verify", "<input type=password name=PassVerify>"))
    tab += tuple (UsersHelper.form_cell_vals (itemCells, data))

    tab += ((recaptcha_display (InstDB.ReCaptchaPubKey, False, recaptcha_error),),)

    tab += (('<input type=submit name=Register value="Register">\n'
           '<a href="patches.py?Action=help&amp;Topic=privacy&amp;Mini=1"'
            ' target="_blank">Privacy Policy</a>',),)

    # Display register form
    box = InstDB.html_box ()
    req.write (box.start ("User Registration"))
    req.write ("<table border=0 cellpadding=8>\n<tr>\n<td valign=top>\n");
    req.write ("<form action=\"patches.py?Action=register\" method=POST>\n")

    split = InstDB.tabsplits (tab)
    split.tableattr = 'bgcolor="#408ee6"'
    split.cellspacing = 4
    req.write (split.render ())

    req.write ("</form>\n");
    req.write ("</td>\n<td valign=top>\n")

    req.write (InstDB.IncFile ("register.inc"))

    req.write ("</td>\n</tr>\n</table>\n")
    req.write (box.end ())

    req.write (InstDB.footer ())
