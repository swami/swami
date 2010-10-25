#!/usr/bin/python
#
# list.py - List patch items.
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
from urllib import urlencode

from PatchItem import *
import InstDB

def action (req, fields):
    Type = fields.get ("Type", "Patch")
    if Type == "Inst": s = "Instruments"
    elif Type == "Sample": s = "Samples"
    else: s = "Patch Files"

    req.write (InstDB.header ("List " + s))

    if Type == "Inst": list_insts (req, fields)
    elif Type == "Sample": list_samples (req, fields)
    else: list_patches (req, fields)

    req.write (InstDB.footer ())


def list_patches (req, fields):
    OrderBy = fields.get ("OrderBy", None)
    Category = int (fields.get ("Category", 0))
    CategoryExact = int (fields.get ("CategoryExact", 0))

    OrderDir = int (fields.get ("OrderDir", 0))
    if OrderDir < 0 or OrderDir > 2: OrderDir = 0

    Offset = int (fields.get ("Offset", 0))
    Count = int (fields.get ("Count", 50))

    if Count <= 5: Count = 5
    elif Count > InstDB.MaxListItems: Count = InstDB.MaxListItems

    itemFields = list (PatchHelper.list_format)

    # Category specified?
    if Category:
        CategoryIds = (str(Category),)
        if not CategoryExact:  # Exact category NOT specified?
            InstDB.Cur.execute ("SELECT CategoryName FROM Category"
                                " WHERE CategoryId=%s", Category) # Name
            res = InstDB.Cur.fetchone ()
            if res:   # Get list of sub categories (if any)
                InstDB.Cur.execute ("SELECT CategoryId FROM Category"
                                    " WHERE LEFT(CategoryName, %s) = %s",
                                    (len (res["CategoryName"]) + 1,
                                     res["CategoryName"] + ':'))
                for row in InstDB.Cur.fetchall():
                    CategoryIds += (str(row["CategoryId"]),)

        # Remove category from item fields
        if "CategoryName" in itemFields:
            itemFields.remove ("CategoryName")

    # Create SQL query
    sel = PatchHelper.sql_select (itemFields)

    # Make sure order by field is valid
    if OrderBy and not PatchHelper.field_searchable (OrderBy): OrderBy = None
    if not OrderBy: OrderBy = "FileName"

    sel.orderby = PatchHelper.sql_order_field (OrderBy, OrderDir)
    sel.limit = "%d,%d" % (Offset, Count)
    sel.where.append ("PatchInfo.State = 'Active'")
    sel.calc_found_rows = True

    if Category:
        # Match categories (1st, 2nd or 3rd)
        s = string.join (CategoryIds, ',')
        sel.where.insert (0, "(PatchInfo.CategoryId IN (%s)"
                          " || PatchInfo.CategoryId2 IN (%s)"
                          " || PatchInfo.CategoryId3 IN (%s))" % (s, s, s))
        if not "PatchInfo.CategoryId" in sel.fields:
            sel.fields += ("PatchInfo.CategoryId",)

    InstDB.Cur.execute (sel.query ())
    rows = InstDB.Cur.fetchall ()

    # Get total number of matching items
    InstDB.Cur.execute ("SELECT FOUND_ROWS() AS Total");
    row = InstDB.Cur.fetchone ()
    total = int (row["Total"])

    # Re URL-ify the input fields
    pager_urlstr = urlencode ((("Action", "list"), ("Type", "Patch"),
                               ("Category", Category), ("OrderBy", OrderBy),
                               ("OrderDir", OrderDir), ("Count", Count)))
    pager_urlstr = cgi.escape (pager_urlstr)

    # Display the pager bar
    InstDB.show_pager (req, pager_urlstr, Offset, Count, total)

    # Create clickable titles that sort output
    urlstr = urlencode ((("Action", "list"), ("Type", "Patch"),
                         ("Category", Category), ("Count", Count)))
    urlstr = cgi.escape (urlstr)

    sortList = []
    for field in itemFields:
        if PatchHelper.field_searchable (field):
            sortList += ((field, PatchHelper.field_title (field)),)
        else: sortList += (PatchHelper.field_title (field),)

    titles = sort_titles (sortList, urlstr, OrderBy, OrderDir, PatchHelper)

    if rows:
        table = InstDB.tabular (titles)
        table.tableattr = 'width="100%"'

        for row in rows:
            req.write (table.addrow (PatchHelper.html_vals (itemFields, row)))

        req.write (table.end ())
    else:
        req.write ("<b>No files match criteria</b>\n")

    # Display the pager bar
    InstDB.show_pager (req, pager_urlstr, Offset, Count, total)


def list_insts (req, fields):
    OrderBy = fields.get ("OrderBy", None)

    OrderDir = int (fields.get ("OrderDir", 0))
    if OrderDir < 0 or OrderDir > 2: OrderDir = 0

    Offset = int (fields.get ("Offset", 0))
    Count = int (fields.get ("Count", 50))
    PatchId = int (fields["PatchId"])

    if Count <= 5: Count = 5
    elif Count > InstDB.MaxListItems: Count = InstDB.MaxListItems

    InstDB.Cur.execute ("SELECT State, UserId FROM PatchInfo WHERE PatchId=%s",
                        PatchId)

    row = InstDB.Cur.fetchone ()
    if not row:
        InstDB.error (req, "Specified patch file not found")
        return

    # Only allow list if patch is active, user is admin or user owns imported file
    if row["State"] != "Active":
        if not InstDB.User or ("Admin" not in InstDB.User["Flags"] and \
              (row["State"] != 'Imported' or InstDB.User["UserId"] != row["UserId"])):
            InstDB.error (req, "Permission denied to specified patch file")
            return            

    itemFields = InstHelper.list_format
    extraFields = ("State", "PatchId")

    # Create SQL query
    sel = InstHelper.sql_select (itemFields + extraFields)

    # Make sure order by field is valid
    if OrderBy and not InstHelper.field_searchable (OrderBy): OrderBy = None
    if not OrderBy: OrderBy = "InstName"

    sel.orderby = InstHelper.sql_order_field (OrderBy, OrderDir)
    sel.limit = "%d,%d" % (Offset, Count)
    sel.where.insert (0, "PatchInfo.PatchId=%d" % PatchId)
    sel.calc_found_rows = True

    InstDB.Cur.execute (sel.query ())
    rows = InstDB.Cur.fetchall ()

    # Get total number of matching items
    InstDB.Cur.execute ("SELECT FOUND_ROWS() AS Total")
    row = InstDB.Cur.fetchone ()
    total = int (row["Total"])

    # Re URL-ify the input fields
    pager_urlstr = urlencode ((("Action", "list"), ("Type", "Inst"),
                               ("OrderBy", OrderBy), ("OrderDir", OrderDir),
                               ("Count", Count), ("PatchId", PatchId)))
    pager_urlstr = cgi.escape (pager_urlstr)

    # Display the pager bar
    InstDB.show_pager (req, pager_urlstr, Offset, Count, total)

    # Create clickable titles that sort output
    urlstr = urlencode ((("Action", "list"), ("Type", "Inst"),
                         ("Count", Count), ("PatchId", PatchId)))
    urlstr = cgi.escape (urlstr)

    sortList = []
    for field in itemFields:
        if InstHelper.field_searchable (field):
            sortList += ((field, InstHelper.field_title (field)),)
        else: sortList += (InstHelper.field_title (field),)

    titles = sort_titles (sortList, urlstr, OrderBy, OrderDir, InstHelper)

    table = InstDB.tabular (titles)
    table.tableattr = 'width="100%"'

    for row in rows:
        req.write (table.addrow (InstHelper.html_vals (itemFields, row)))

    req.write (table.end ())

    # Display the pager bar
    InstDB.show_pager (req, pager_urlstr, Offset, Count, total)


def list_samples (req, fields):
    OrderBy = fields.get ("OrderBy", None)

    OrderDir = int (fields.get ("OrderDir", 0))
    if OrderDir < 0 or OrderDir > 2: OrderDir = 0

    Offset = int (fields.get ("Offset", 0))
    Count = int (fields.get ("Count", 50))
    PatchId = int (fields.get ("PatchId", 0))

    if Count <= 5: Count = 5
    elif Count > InstDB.MaxListItems: Count = InstDB.MaxListItems

    InstDB.Cur.execute ("SELECT State, UserId FROM PatchInfo WHERE PatchId=%s",
                        PatchId)

    row = InstDB.Cur.fetchone ()
    if not row:
        InstDB.error (req, "Specified patch file not found")
        return

    # Only allow list if patch is active, user is admin or user owns imported file
    if row["State"] != "Active":
        if not InstDB.User or ("Admin" not in InstDB.User["Flags"] and \
              (row["State"] != 'Imported' or InstDB.User["UserId"] != row["UserId"])):
            InstDB.error (req, "Permission denied to specified patch file")
            return            

    itemFields = SampleHelper.list_format
    extraFields = ("State",)
    if PatchId: extraFields += ("PatchId",)

    # Create SQL query
    sel = SampleHelper.sql_select (itemFields + extraFields)

    # Make sure order by field is valid
    if OrderBy and not SampleHelper.field_searchable (OrderBy): OrderBy = None
    if not OrderBy: OrderBy = "SampleName"

    sel.orderby = SampleHelper.sql_order_field (OrderBy, OrderDir)
    sel.limit = "%d,%d" % (Offset, Count)
    if PatchId: sel.where.insert (0, "PatchInfo.PatchId=%d" % PatchId)
    sel.calc_found_rows = True

    InstDB.Cur.execute (sel.query ())
    rows = InstDB.Cur.fetchall ()

    # Get total number of matching items
    InstDB.Cur.execute ("SELECT FOUND_ROWS() AS Total");
    row = InstDB.Cur.fetchone ()
    total = int (row["Total"])

    # Re URL-ify the input fields
    pager_urlstr = urlencode ((("Action", "list"), ("Type", "Sample"),
                               ("OrderBy", OrderBy), ("OrderDir", OrderDir),
                               ("Count", Count), ("PatchId", PatchId)))
    pager_urlstr = cgi.escape (pager_urlstr)

    # Display the pager bar
    InstDB.show_pager (req, pager_urlstr, Offset, Count, total)

    # Create clickable titles that sort output
    urlstr = urlencode ((("Action", "list"), ("Type", "Sample"),
                         ("Count", Count), ("PatchId", PatchId)))
    urlstr = cgi.escape (urlstr)

    sortList = []
    for field in itemFields:
        if SampleHelper.field_searchable (field):
            sortList += ((field, SampleHelper.field_title (field)),)
        else: sortList += (SampleHelper.field_title (field),)

    titles = sort_titles (sortList, urlstr, OrderBy, OrderDir, SampleHelper)

    table = InstDB.tabular (titles)
    table.tableattr = 'width="100%"'

    for row in rows:
        req.write (table.addrow (SampleHelper.html_vals (itemFields, row)))

    req.write (table.end ())

    # Display the pager bar
    InstDB.show_pager (req, pager_urlstr, Offset, Count, total)


def sort_titles (titles, urlstr, orderby, orderdir, helper):
    ret = ()
    link = '<a href="patches.py?%s&amp;OrderBy=%s">%s</a>\n'

    for title in titles:
        if type (title) == str: ret += (title,)
        elif orderby != title[0]: ret += (link % (urlstr, title[0], title[1]),)
        else:
            deforder = helper.field_searchable (title[0])
            if orderdir == 0: orderdir = deforder

            # Currently default order? Clicking will alternate it.
            if orderdir == deforder:
                if orderdir == 2: altorder = 1
                else: altorder = 2

                dirstr = "&amp;OrderDir=%d" % altorder
            else: dirstr = ""

            if orderdir == 2:
              arrow = "down"
              alt = "Descending"
            else:
              arrow = "up"
              alt = "Ascending"

            ret += ('[%s]' % title[1]
                    + '<a href="patches.py?%s&amp;OrderBy=%s%s">'
                    % (urlstr, title[0], dirstr)
                    + '<img src="images/arrow_%s.png" alt="%s"' % (arrow, alt)
                    + ' width=16 height=14 border=0></a>',)

    return ret
