#!/usr/bin/python
#
# confirm.py - Email confirmations for registration and password reset.
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

import sha
import datetime
import random
import os
import phpbb
import InstDB
from mod_python import apache


def new (action, userid=None, login=None):
    """
    Create a new email confirmation, which includes generating a hash,
    adding it to the Confirm table and sending the user an email containing
    a link for following up on the confirmation.

    action -- Type of confirmation ('Register' or 'ResetPass')
    userid -- optional - ID of user (InstDB.User is used if not specified)
    login -- optional - User's login (InstDB.User if not specified)
    """

    if not userid: userid = InstDB.User["UserId"]
    if not login: login = InstDB.User["Login"]

    # Throw a bunch of crap at the SHA1 hasher (overkill?  Nahh never!)
    hash = sha.new ()
    hash.update (str (datetime.datetime.now ()))   # Current time w/ microsecs
    hash.update (str (InstDB.User))      # Hash stringified user settings
    hash.update (InstDB.FuKraXor)        # Want a piece of this?
    hash.update (str (random.random ())) # How about a random number for fun

    hexkey = hash.hexdigest ()

    InstDB.Cur.execute ("INSERT INTO Confirm (UserId, ConfirmAction, HashKey)"
                        " VALUES(%s, %s, %s)", (userid, action, hexkey))

    if action == "Register": fname = "register.msg"
    elif action == "ResetPass": fname = "resetpass.msg"

    f = open (InstDB.IncludePath + os.sep + fname)
    email = f.read () % {
        "Login" : login,
        "HashKey" : hexkey }

    # Queue email task
    InstDB.Cur.execute ("INSERT INTO Queue (Type, Status, UserId, Content)"
                        " VALUES ('Email', 'Queued', %s, %s)", (userid, email))

def action (req, fields):
    msg = fields.get ("Msg", None)

    if msg == "Registered":
        req.write (InstDB.header ("Email confirmation"))
        req.write (InstDB.IncFile ("activated.inc"))
        req.write (InstDB.footer ())
        return
    elif msg == "Reset":
        req.write (InstDB.header ("Email confirmation"))
        req.write ('<font class="title">Password has been reset and will be'
                   ' emailed to you shortly.</font>\n')
        req.write (InstDB.footer ())
        return

    login = fields.get ("Login", None)
    hashkey = fields.get ("HashKey", None)

    if not login or not hashkey:
        req.write (InstDB.header ("Email confirmation"))
        InstDB.error (req, "Invalid confirmation data")
        req.write (InstDB.footer ())
        return

    InstDB.Cur.execute ("SELECT UserId FROM Users WHERE Login=%s", login)
    row = InstDB.Cur.fetchone ()
    if not row:
        req.write (InstDB.header ("Email confirmation"))
        InstDB.error (req, "Invalid confirmation data")
        req.write (InstDB.footer ())
        return

    userid = row["UserId"]

    InstDB.Cur.execute ("SELECT ConfirmAction FROM Confirm"
                        " WHERE UserId=%s && HashKey=%s", (userid, hashkey))

    row = InstDB.Cur.fetchone ()
    if not row:
        req.write (InstDB.header ("Email confirmation"))
        InstDB.error (req, "Email confirmation not found, perhaps it expired?")
        req.write (InstDB.footer ())
        return

    if row["ConfirmAction"] == "Register":
        InstDB.Cur.execute ("UPDATE Users SET Flags='' WHERE UserId=%s", userid)

        if InstDB.EnableForums:
          retval = phpbb.activate_user (login)  # Activate the forum account

          if retval != 0:
            raise RuntimeWarning, "Failed to activate PHPBB user '%s' (retval=%d)" % (login, retval)

        # Redirect to confirm page with Msg=Registered parameter
        if req.proto_num >= 1001:
            req.status = apache.HTTP_TEMPORARY_REDIRECT
        else: req.status = apache.HTTP_MOVED_TEMPORARILY

        req.headers_out["Location"] = "patches.py?Action=confirm&amp;Msg=Registered"
        req.send_http_header()

    elif row["ConfirmAction"] == "ResetPass":
        newpass = ""
        for i in range (0, 8):
            newpass += random.choice ("ABCDEFGHIJKLMNPQRSTUVWXYZ"
                                      "abcdefghjkmnpqrstuvwxyz23456789")

        InstDB.Cur.execute ("UPDATE Users SET Password=SHA1(%s)"
                            " WHERE UserId=%s", (newpass + InstDB.FuKraXor, userid))

        f = open (InstDB.IncludePath + os.sep + "newpass.msg")
        email = f.read () % {
            "Login" : login,
            "Password" : newpass }

        # Queue email task
        InstDB.Cur.execute ("INSERT INTO Queue (Type, Status, UserId, Content)"
                            " VALUES ('Email', 'Queued', %s, %s)",
                            (userid, email))

        # Redirect to confirm page with Msg=Reset parameter
        if req.proto_num >= 1001:
            req.status = apache.HTTP_TEMPORARY_REDIRECT
        else: req.status = apache.HTTP_MOVED_TEMPORARILY

        req.headers_out["Location"] = "patches.py?Action=confirm&amp;Msg=Reset"
        req.send_http_header()


    # Remove the confirmation, we done with it
    InstDB.Cur.execute ("DELETE FROM Confirm WHERE UserId=%s && HashKey=%s",
                        (userid, hashkey))
