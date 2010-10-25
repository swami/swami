#!/usr/bin/python
#
# help.py - Help information.
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

topic_titles = { "submit" : "Content submission help", "privacy" : "Privacy policy" }

def action (req, fields):
    topic = fields.get ("Topic", None)

    mini = fields.get ("Mini", '0')
    if mini != '0': mini = True
    else: mini = False

    title = topic_titles.get (topic, "Help")

    req.write (InstDB.header (title, mini))

    if not topic_titles.get (topic, None):
        InstDB.error (req, "Help topic not found!")
        req.write (InstDB.footer (mini))
        return

    box = InstDB.html_box ()
    req.write (box.start (title))
    req.write (InstDB.IncFile ("%s.inc" % topic))
    req.write (box.end ())

    req.write (InstDB.footer (mini))
