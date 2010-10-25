#!/usr/bin/python
#
# InstDB.py - Utility functions for web output and database
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
import MySQLdb
from math import *
from cgi import escape

from config import *

RatingRadio = ( ("0","None"), "1", "2", "3", "4", "5" )
LoginError = ""

def connect ():
    global DB, Cur

    DB = MySQLdb.connect (host=dbHost, db=dbName, user=dbUser, passwd=dbPass)
    Cur = DB.cursor (MySQLdb.cursors.DictCursor)


def header(title=None, minimal=False):
    global LoginError

    if title: s = " - " + title
    else: s = ""

    retstr = """\
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd">
<html>
<head>
<title>Resonance Instrument DB%s</title>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<meta name="keywords" content="instruments midi music sounds SoundFont DLS GigaSampler">
<meta name="description" content="Database for sharing instrument sound files for computer music composition.">
<link REL="StyleSheet" HREF="images/style.css" TYPE="text/css">

<script type="text/javascript">
var RecaptchaOptions = {
   theme : 'blackglass'
};
</script>

</head>
<body bgcolor="#000000">
""" % s

    if not minimal:
        retstr += """\
<center><img src="images/logo.jpg" width=420 height=143 alt="Resonance Instrument Database logo"></center>

<table border=0 cellpadding=4 cellspacing=4 width="100%">
<tr>
<td align="left" valign="top" width="160">
"""

        box = html_box ()
        box.tableattr = 'width="100%"'
        retstr += box.start ("Menu") + """\
<table border=0 cellpadding=2 cellspacing=0 width="100%">
<tr><td>
<a href="patches.py">Home</a><br>
<a href="forums/">Forums</a><br>
<a href="patches.py?Action=software">Software/Credits</a><br>
</td></tr>
<tr bgcolor="#000066"><td align="center">
<b>Instruments</b>
</td></tr>
<tr><td>
<a href="patches.py?Action=browse">Browse&nbsp;by&nbsp;Category</a><br>
<a href="patches.py?Action=list&amp;OrderBy=Date">New&nbsp;Files</a><br>
<a href="patches.py?Action=list&amp;OrderBy=FileNameLink">List&nbsp;by&nbsp;FileName</a><br>
<a href="patches.py?Action=md5find">Find&nbsp;by&nbsp;MD5</a>
</td></tr>
<tr bgcolor="#000066"><td align="center">
<b>Documentation</b>
</td></tr>
<tr><td>
<a href="patches.py?Action=help&amp;Topic=submit">Content&nbsp;Submission</a><br>
<a href="patches.py?Action=help&amp;Topic=privacy">Privacy&nbsp;Policy</a><br>
</td></tr>
<tr bgcolor="#000066"><td align="center">
<b>Search</b>
</td></tr>
<tr><td align="center">
<form action="patches.py" method=GET>
<input type=hidden name=Action value="search">
<input style="font-size: 10px;" type=text name=Text size=16 maxlength=64>
</form>
<p>
</td></tr>
<tr bgcolor="#000066"><td align="center">
"""

        if User:
            retstr += "<b>Hello '%s'" % User['Login'] + """</b>
</td></tr>
<tr><td>
<a href="patches.py?Action=profile">Edit&nbsp;Profile</a><br>
<a href="patches.py?Action=submit">Submit&nbsp;Content</a><br>
"""

            if "Admin" in User["Flags"]:
                retstr += """
<a href="patches.py?Action=emailnews">Email&nbsp;News</a><br>
"""

            retstr += """
<a href="patches.py?Action=logout">Logout</a><br>
</td></tr>
"""
        else:
            retstr += """\
<b>Login</b>
</td></tr>
<tr><td align="center">
<form action="" method=POST>
<input style=\"font-size: 10px;\" type=text name=Username size=16 maxlength=24><br>
Password<br>
<input style=\"font-size: 10px;\" type=password name=Pass size=16 maxlength=24>
<p>
<input type=submit value=\"Login\">&nbsp;&nbsp;<a href=\"patches.py?Action=register\">Register</a>
<p>
<a href="patches.py?Action=resetpass">Forgot password?</a><br>
</form>
</td></tr>
"""

        if LoginError:
            retstr += '<tr><td><font class=err>%s</font></td></tr>\n' % LoginError
    
        retstr += "</table>" + box.end () + '</td>\n<td valign="top">\n'

        # END - if not minimal

    return retstr

def footer (minimal=False):

    if not minimal: return "</td>\n</tr>\n</table>\n</body>\n</html>\n"
    else: return "</body>\n</html>\n"

class html_box:
    def __init__ (self):
        self.ttlpadding = 2
        self.boxpadding = 4
        self.tableattr = None
        self.titlehtml = ""
        self.brdrcolor = "#004b9b"
        self.bgcolor = "#00001e"

    def start (self, title):
        if self.bgcolor: bgcolor = ' bgcolor="%s"' % self.bgcolor
        else: bgcolor = ""

        if self.tableattr: tableattr = ' ' + self.tableattr
        else: tableattr = ""

        return """\
<table border=0 cellpadding=%d cellspacing=0 bgcolor="%s"%s>
<tr bgcolor="%s">
<td align="left" style="background-image:url('images/storbg.gif');">
<font class="boxtitle">%s</font>%s
</td>
</tr>
<tr><td>
<table width="100%%" border=0 cellpadding=%d cellspacing=0>
<tr%s><td>
""" % (self.ttlpadding, self.brdrcolor, tableattr, self.brdrcolor,
       escape (title), self.titlehtml, self.boxpadding, bgcolor)

    def end (self):
        return """\
</td></tr>
</table>
</td></tr>
</table>
"""

class tabular:
    "Tabular HTML output class"

    def __init__ (self, titles=['']):
        self.titles = titles
        self.rowalt = 0
        self.colattr = []
        self.cellspacing = 0
        self.cellpadding = 2
        self.tableattr = ""
        self.ttlrowattr = "bgcolor=\"#110088\""
        self.bgcolor1 = "#000000"
        self.bgcolor2 = "#111111"

        # private
        self.displayed_titles = False

    def addrow (self, values):
        if not self.displayed_titles: s = self.showtitles ()
        else: s = ""

        if len (values) != len (self.titles):
            raise ValueError, "Input values don't match title count"

        s += "<tr bgcolor=\"%s\">\n" % (self.bgcolor1,
                                        self.bgcolor2)[self.rowalt]
        self.rowalt ^= 1

        i = 0
        for val in values:
            if len (self.colattr) > i and self.colattr[i]:
                attr = " " + self.colattr[i]
            else: attr = ""
            s += "<td%s>%s</td>\n" % (attr, val)
            i += 1

        s += "</tr>\n"

        return s

    def showtitles (self):
        self.displayed_titles = True

        if self.ttlrowattr: ttlattr = " " + self.ttlrowattr
        else: ttlattr = ""

        if self.tableattr: tabattr = " " + self.tableattr
        else: tabattr = ""

        s = '<table border=0 cellpadding="%s" cellspacing="%s"%s>\n' \
            '<tr%s>\n' % (self.cellpadding, self.cellspacing, tabattr, ttlattr)
        i = 0
        for title in self.titles:
            if len (self.colattr) > i and self.colattr[i]:
                attr = " " + self.colattr[i]
            else: attr = ""
            s += '<td%s><b>%s</b></td>\n' % (attr, title)
            i += 1
        return s + '</tr>\n'

    def end (self):
        s = ""
        if not self.displayed_titles: s += self.showtitles ()
        return s + '</table>\n'


class tabsplits:
    "Tabular HTML splits (Name/Value pairs)"

    def __init__ (self, rows):
        self.rows = rows                 # sequence of sequences for each cell
        self.cellspacing = 2
        self.cellpadding = 2
        self.tableattr = ""              # extra HTML attributes for <table>
        self.rowattr = []                # sequence of HTML attributes for rows
        self.cellattr = []               # sequence of HTML attributes for cells
        self.keybgcolor = "#110088"      # "Key" background color
        self.valbgcolor = "#110044"      # "Value" background color
        self.skeytag = "<b>"
        self.ekeytag = "</b>"
        self.svaltag = ""
        self.evaltag = ""

    def render (self):
        if self.tableattr: self.tableattr = " " + self.tableattr
        else: self.tableattr = ""

        s = '<table border=0 cellpadding="%s" cellspacing="%s"%s>\n' % (
            self.cellpadding, self.cellspacing, self.tableattr)

        # Find the maximum number of columns in all rows
        maxcol = 0
        for row in self.rows:
            if len (row) > maxcol: maxcol = len (row)

        rowindex = 0
        totalcellindex = 0
        for row in self.rows:           # loop over rows
            # Get row HTML attribute if any
            if rowindex < len (self.rowattr) and self.rowattr[rowindex]:
                rattr = " " + self.rowattr[rowindex]
            else: rattr = ""

            s += '<tr%s>\n' % rattr

            cellalt = 0
            cellindex = 0
            cellcount = len (row)
            for cell in row:            # loop over cells in each row
                # Add a colspan attribute if less than max row cells
                if cellindex == cellcount - 1 and cellcount < maxcol:
                    cattr = ' colspan="%d"' % (maxcol - cellcount + 1)
                else: cattr = ""

                # Get cell HTML attribute if any
                if totalcellindex < len (self.cellattr) \
                       and self.cellattr[totalcellindex]:
                    cattr += " " + self.cellattr[totalcellindex]

                s += '<td bgcolor="%s"%s>%s%s%s</td>\n' \
                     % ((self.keybgcolor, self.valbgcolor)[cellalt], cattr,
                        (self.skeytag, self.svaltag)[cellalt], cell,
                        (self.ekeytag, self.evaltag)[cellalt])
                cellindex += 1
                totalcellindex += 1
                cellalt ^= 1

            s += '</tr>\n'
            rowindex += 1

        s += '</table>\n'
        return s

def cells_to_fields (cells):
    "Convert field cells (array of array of strings) to fields (array of strings)"
    fields = []
    for cell in cells:
        for field in cell:
            fields += (field,)
    return fields

def delete_cells (cells, fields):
    "Remove fields from a cell list (list of lists)"
    newcells = []
    for cell in cells:
        newcell = []
        for field in cell:
            if not field in fields:
                newcell += (field,)
        if newcell:
            newcells += (newcell,)
    return newcells

def form_radio (name, values, selected=None):
    """Outputs HTML form radio buttons
name: HTML name to use for radio buttons
values: Tuple of value strings (also label) or value:label tuples
selected: 'value' for the currently selected radio button
"""
    s = ""
    for btn in values:
        if type (btn) == str:
            value = btn
            label = btn
        else:
            value = btn[0]
            label = btn[1]

        if value == selected:
            checked = " checked"
        else: checked = ""

        s += "<input type=radio name=\"%s\" value=\"%s\"%s>%s\n" \
             % (escape (name), escape (value), checked, escape (label))

    return s


def pretty_size (size):
    if size == 0: return "0"

    m = int (floor (log (size) / log (1024)))
    div = 1024 ** m
    s = ('b', 'k', 'M', 'G')[m]
    if m >= 2: prec = 2
    else: prec = m
    return "%.*f%s" % (prec, size / float (div), s)


def show_pager (req, urlstr, offset, pagesize, total, maxpages=20):
    pages = (total + pagesize - 1) / pagesize
    if pages <= 1: return               # Don't display pager if only 1 page

    curpage = offset / pagesize

    req.write ("""\
<table border=0 cellpadding=0 cellspacing=0 bgcolor=\"#110044\" width=\"100%\">
<tr><td>
<center>
<table border=0 cellpadding=0 cellspacing=2>
<tr>
""")

    link = "<td>\n<a href=\"patches.py?%s&amp;Offset=%d\">\n" \
           "<b><font size=\"+1\">%s</font></b>\n</a>\n</td>\n"
    txt = '<td><b><font size="+1">%s</font></b></td>\n'

    # '|<' First page link
    if offset != 0: req.write (link % (urlstr, 0, "|&lt;"))
    else: req.write (txt % "|&lt;")

    # Previous link
    if offset > 0: req.write (link % (urlstr,max (0, offset - pagesize),"&lt;"))
    else: req.write (txt % "&lt;")

    # Numbered pages
    start = curpage - curpage % maxpages
    end = min (start + maxpages, pages)

    if start > 0: req.write (txt % "...")

    for p in range (curpage - curpage % maxpages, end):
        if p != curpage: req.write (link % (urlstr, p * pagesize, p + 1))
        else: req.write (txt % (p + 1))

    if end < pages:
        req.write (txt % "...")
        req.write (link % (urlstr, (pages - 1) * pagesize, pages))

    # Next link
    if offset + pagesize < total:
        req.write (link % (urlstr, offset + pagesize, "&gt;"))
    else: req.write (txt % "&gt;")

    # '|<' Last page link
    if pages > 0 and offset != (pages - 1) * pagesize:
        req.write (link % (urlstr, (pages - 1) * pagesize, "&gt;|"))
    else: req.write (txt % "&gt;|")

    req.write ("""\
</tr>
</table>
</center>
</tr>
</table>
""")


def show_status (req, str):
    """Display an HTML status message"""
    req.write ("<center>\n<b>%s</b>\n</center>\n<hr>\n<p>\n" % str)


def sqlescape (str):
    """Escape a string for use in an SQL statement"""
    s = string.replace (str, "'", "\\'")
    return string.replace (str, "\"", "\\\"")

def emailmunge (str):
    return string.replace (str, "@", " <at> ")

def IncFile (filename):
    global IncludePath
    f = open (IncludePath + "/" + filename)
    return f.read ()

def AssertLogin (req, msg="Please login to access this page"):
    global User

    if not User:
        req.write ('<font class="error">%s</font>\n' % msg)
        return False
    else: return True

def error(req, msg):
    """Display an HTML error"""
    req.write ('<font class="error">%s</font>\n' % escape (msg))

class Error(Exception):
    """Base class for exceptions in this module."""
    pass

class ImportError(Error):
    """Exception raised on patch file import errors.

    Attributes:
        message -- explanation of the error
    """

    def __init__(self, message):
        self.message = message

    def __str__(self):
        return (repr (self.message))
