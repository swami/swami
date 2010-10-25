#!/usr/bin/python
#
# patchesHandler.py - Contains the mod_python handler and user login function.
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

from mod_python import apache
from urllib import urlencode
from mod_python import util
from mod_python import Session
from mod_python import apache
import MySQLdb

import InstDB

def init(req):
    InstDB.Req = req
    InstDB.Fields = util.FieldStorage (req)

    InstDB.connect ()

    InstDB.User = None
    InstDB.Sess = Session.Session (req)
    if InstDB.Sess.is_new ():
        login = InstDB.Fields.get ("Username", None)
        passwd = InstDB.Fields.get ("Pass", None)

        if login and passwd:    # lookup user by login name and password
            InstDB.Cur.execute ("SELECT * FROM Users WHERE Login=%s"
                                " && Password=SHA1(%s)"
                                " && !FIND_IN_SET('NoLogin', Flags)"
                                " && !FIND_IN_SET('Confirm', Flags)",
                                (login, passwd + InstDB.FuKraXor))
            InstDB.User = InstDB.Cur.fetchone ()
            if InstDB.User:
                InstDB.Cur.execute ("UPDATE Users SET LastLogin=NULL"
                                    " WHERE UserId=%s" % InstDB.User["UserId"])

                InstDB.Sess["UserId"] = InstDB.User["UserId"]
                InstDB.Sess.set_timeout (InstDB.SessTimeout)
                InstDB.Sess.save ()

                InstDB.LoginError = ""
            else: InstDB.LoginError = "Invalid login"

        if not InstDB.Sess.has_key ("UserId"): InstDB.Sess["UserId"] = 0
    else:                               # Sess is not new
        InstDB.Sess.load ()               # Load session values
        InstDB.Cur.execute ("SELECT * FROM Users WHERE UserId=%s"
                            " && !FIND_IN_SET('NoLogin', Flags)",
                            (InstDB.Sess["UserId"],))
        InstDB.User = InstDB.Cur.fetchone ()
        if InstDB.User:
          del InstDB.User["Password"]  # no need to keep it around
          InstDB.LoginError = ""
        else:
            InstDB.Sess["UserId"] = 0
            InstDB.Sess.delete ()

def shutdown ():
    InstDB.Cur.close ()
    InstDB.DB.close ()

def handler (req):
    InstDB.DebugReq = req

    init (req)

    req.content_type = "text/html"

    dlfile = "download.py"

    # Was download.py specified?
    if req.filename[-len(dlfile):] == dlfile:
        action = "dnld"
    else: action = InstDB.Fields.get ("Action", None)

    # Invalidate the session if user requested logout
    if action == "logout" and InstDB.User:
        InstDB.Sess.invalidate ()
        InstDB.User = None
        InstDB.Sess["UserId"] = 0

        # Redirect to home page
        if req.proto_num >= 1001:
            req.status = apache.HTTP_TEMPORARY_REDIRECT
        else: req.status = apache.HTTP_MOVED_TEMPORARILY

        req.headers_out["Location"] = "patches.py"
        req.send_http_header()
        return apache.OK
    elif action not in ("browse", "list", "item", "edit", "search", "register",
                        "dnld", "submit", "software", "help",
                        "profile", "resetpass", "confirm", "dbinit",
                        "emailnews", "md5find"):
        action = "index"

    mod = apache.import_module (action)
    mod.action (req, InstDB.Fields)

    shutdown ()

    return apache.OK
