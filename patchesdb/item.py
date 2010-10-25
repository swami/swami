#!/usr/bin/python
#
# item.py - Display information on a patch item.
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
import string
from PatchItem import *
import InstDB

from recaptcha.client.captcha import displayhtml as recaptcha_display
from recaptcha.client.captcha import submit as recaptcha_submit

def action (req, fields):
    id = int (fields.get ("ItemId", 0))

    req.write (InstDB.header ("Patch file details"))

    InstDB.Cur.execute ("SELECT State,UserId FROM PatchInfo WHERE PatchId=%s", id)
    row = InstDB.Cur.fetchone ()
    if not row:
        InstDB.error (req, "Patch file with ID '%d' not found" % id)
        req.write (InstDB.footer ())
        return

    # See if we are allowed to view this patch
    if row["State"] != "Active" \
            and (not InstDB.User or not (row["UserId"] == InstDB.User["UserId"]
                                         or "Admin" in InstDB.User["Flags"])):
        InstDB.error (req, "You don't have permission to view that info")
        req.write (InstDB.footer ())
        return

    show_patch_item (id, fields, req)

    req.write (InstDB.footer ())



def show_patch_item (id, fields, req):
    RateOrder = fields.get ("RateOrder", "Date")

    OrderDir = int (fields.get ("OrderDir", 0))
    if OrderDir < 0 or OrderDir > 2: OrderDir = 0

    recaptcha_error = None
    comment = ""

    # Check for comment/rating submission
    if fields.get ("submit", None):
        comment = fields.get ("comment", "")
        captcha_challenge = fields.get ("recaptcha_challenge_field", "")
        captcha_response = fields.get ("recaptcha_response_field", "")

        if InstDB.User:                 # User is logged in?
            rating = int (fields.get ("rating", 0))
            if rating not in range (0, 6): rating = 0

            # Select existing comment/rating if any
            InstDB.Cur.execute ("SELECT Rating, Comment FROM Ratings"
                                " WHERE PatchId=%s && UserId=%s",
                                (id, InstDB.User["UserId"]))
            row = InstDB.Cur.fetchone ()
            if row:                     # Existing comment/rating?
                InstDB.Cur.execute ("UPDATE Ratings SET Rating=%s, Comment=%s"
                                    " WHERE PatchId=%s && UserId=%s",
                                    (rating, comment, id,
                                     InstDB.User["UserId"]))

                # Calculate increment value for RateCount (user count)
                if rating != 0 and row["Rating"] == 0: countinc = 1 # inc
                elif rating == 0 and row["Rating"] != 0: countinc = -1 # dec
                else: countinc = 0      # Don't change

                # Update RateTotal (+ new rating - old) and RateCount
                InstDB.Cur.execute ("UPDATE PatchInfo SET RateTotal=RateTotal"
                                    " + %s - %s, RateCount=RateCount + %s"
                                    " WHERE PatchId=%s",
                                    (rating, row["Rating"], countinc, id))

                InstDB.show_status (req, "Updated comment/rating data")

            else:                       # Insert new comment/rating
                InstDB.Cur.execute ("INSERT INTO Ratings (PatchId, UserId,"
                                    " Rating, Comment) VALUES(%s,%s,%s,%s)",
                                    (id, InstDB.User["UserId"], rating,
                                     comment))
                if rating != 0:  # Update RateTotal and RateCount for patch file
                    InstDB.Cur.execute ("UPDATE PatchInfo SET RateTotal="
                                        "RateTotal+%s, RateCount=RateCount+1"
                                        " WHERE PatchId=%s", (rating, id))

                InstDB.show_status (req, "Added new comment/rating")

        elif comment:     # User not logged in - add an anonymous comment
            resp = recaptcha_submit (captcha_challenge, captcha_response,
                                     InstDB.ReCaptchaPrvKey, req.connection.remote_ip)
            if resp.is_valid:
                InstDB.Cur.execute ("INSERT INTO Ratings"
                                    " (PatchId, UserId, Comment) VALUES(%s,1,%s)",
                                    (id, comment))
                InstDB.show_status (req, "Anonymous comment added")
                comment = ""
            else:
                InstDB.error (req, "reCAPTCHA response incorrect. Comment not added.")
                recaptcha_error = resp.error_code

    itemCells = PatchHelper.cell_format
    itemFields = InstDB.cells_to_fields (itemCells)

    sel = PatchHelper.sql_select (itemFields)

    # Add UserId field if not already in select query
    if not "PatchInfo.UserId" in sel.fields:
        sel.fields += ("PatchInfo.UserId",)

    sel.where.insert (0, "PatchInfo.State = 'Active'")
    sel.where.insert (0, "PatchInfo.PatchId=%d" % id)

    InstDB.Cur.execute (sel.query ())

    patchRow = InstDB.Cur.fetchone ()
    if not patchRow:       # Shouldn't happen, thats why we raise an exception
        raise LookupError, "Patch file with ID '%d' not found" % id

    fields = InstDB.cells_to_fields (PropHelper.cell_format)
    sel = PropHelper.sql_select (fields)
    sel.where.insert (0, "ItemProps.ItemId=%d" % id)

    InstDB.Cur.execute (sel.query ())
    props = InstDB.Cur.fetchall ()

    # Get count of Instruments in this patch
    InstDB.Cur.execute ("SELECT COUNT(*) AS Count FROM InstInfo"
                        " WHERE PatchId=%s", id)
    instcount = InstDB.Cur.fetchone ()["Count"]

    # Get count of samples in this patch
    InstDB.Cur.execute ("SELECT COUNT(*) AS Count FROM SampleInfo"
                        " WHERE PatchId=%s", id)
    samplecount = InstDB.Cur.fetchone ()["Count"]


    # If user is logged in - see if they have rated this file
    UserRating = None
    if InstDB.User:
        InstDB.Cur.execute ("SELECT Date, Rating, Comment FROM Ratings"
                            " WHERE PatchId=%s && UserId=%s",
                            (id, InstDB.User["UserId"]))
        UserRating = InstDB.Cur.fetchone ()


    # Get ratings/comments
    RatingFields = RatingHelper.list_format

    sel = RatingHelper.sql_select (RatingFields)
    sel.where.insert (0, "PatchId=%d" % id)
    sel.orderby = RatingHelper.sql_order_field (RateOrder, OrderDir)
    sel.limit = str (InstDB.MaxRatings)
    sel.calc_found_rows = True

    InstDB.Cur.execute (sel.query ())
    RatingRows = InstDB.Cur.fetchall ()

    # Get total number of comments/ratings
    InstDB.Cur.execute ("SELECT FOUND_ROWS() AS Total")
    RatingRowsTotal = InstDB.Cur.fetchone()["Total"]

    tab = PatchHelper.cell_vals (itemCells, patchRow)

    if instcount > 0:
        tmptab = ('<a href="patches.py?Action=list&amp;Type=Inst&amp;PatchId=%s'
                  '&amp;OrderBy=Bank">Instruments</a>' % id, instcount)
    else: tmptab = ("Instruments", "None")

    if samplecount > 0:
        tmptab += ('<a href="patches.py?Action=list&amp;Type=Sample&amp;PatchId=%s'
                   '&amp;OrderBy=SampleName">Samples</a>' % id, samplecount)
    else: tmptab += ("Samples", "None")

    tab += (tmptab,)

    tab += add_extra_props (req, props)

    tab += (("<center>Rating &amp; Comment</center>",),)

    if InstDB.User:                     # User logged in?
        if UserRating: tab += (("Modified", UserRating["Date"]),)

        # selected item
        if UserRating: s = str (UserRating["Rating"])
        else: s = "0"

        tab +=(("Rating", InstDB.form_radio ("rating", InstDB.RatingRadio, s)),)
        captcha_code = ""
    else:
        tab += (("Rating", "Login to rate this file"),)
        captcha_code = recaptcha_display (InstDB.ReCaptchaPubKey, False, recaptcha_error)

    if UserRating:
        comment = cgi.escape (UserRating["Comment"])
        submit = "Update"
    else:
        submit = "Submit"


    tab += (("Comment",
             "<textarea name=comment rows=5 cols=40>%s</textarea>\n" % comment
             + captcha_code),
            ("<center>\n<input name=submit type=submit value=%s>\n</center>\n" \
             % submit,))

    # Display patch item info
    box = InstDB.html_box ()

    # User is admin or owns this patch file? - show edit icon
    if InstDB.User and ("Admin" in InstDB.User["Flags"] \
           or patchRow["UserId"] == InstDB.User["UserId"]):
        box.titlehtml = '\n&nbsp;&nbsp;&nbsp;<a href="patches.py?Action=edit&amp;ItemId=%d">' \
                    '<img src="images/edit.png" alt="Edit Mode" width=10 height=15 border=0>' \
                    '&nbsp;<font class="boxtitle">Edit</font></a>\n' % id

    req.write (box.start ("Patch File"))
    req.write ('<form action="" method=POST>\n')

    split = InstDB.tabsplits (tab)
    req.write (split.render ())

    req.write ("</form>\n")
    req.write (box.end ())

    req.write ("<p><p>\n")


    # Show ratings/comments table
    if RatingRows:
        box = InstDB.html_box ()
        req.write (box.start ("Comments"))

        sortList = []
        for field in RatingFields:
            if RatingHelper.field_searchable (field):
                sortList += ((field, RatingHelper.field_title (field)),)
            else: sortList += (RatingHelper.field_title (field),)

        urlstr = urlencode ((("Action", "item"), ("ItemId", str (id))))
        urlstr = cgi.escape (urlstr)

        titles = sort_titles (sortList, urlstr, RateOrder, OrderDir)
        table = InstDB.tabular (titles)
        table.cellpadding = "4"
        table.tableattr = 'width="100%"'

        # Display comments
        for row in RatingRows:
            req.write (table.addrow (RatingHelper.html_vals(RatingFields, row)))

        req.write (table.end ())

        req.write (box.end ())


def add_extra_props (req, props):
    tab = ()
    if props: tab += (('<center>Additional Properties</center>',),)

    # Add all the properties to a hash
    propvals = {}
    for prop in props:
        propvals[string.capitalize (prop["PropName"])] \
            = cgi.escape (InstDB.emailmunge (prop["PropValue"]))

    # Sort the property name keys
    sortkeys = propvals.keys ()
    sortkeys.sort ()

    # Add all the properties to the table
    for key in sortkeys:
        tab += ((key, propvals[key]),)

    return tab


def sort_titles (titles, urlstr, orderby, orderdir):
    ret = ()
    link = '<a href="patches.py?%s&amp;RateOrder=%s">%s</a>\n'

    for title in titles:
        if type (title) == str: ret += (title,)
        elif orderby != title[0]: ret += (link % (urlstr, title[0], title[1]),)
        else:
            deforder = RatingHelper.field_searchable (title[0])
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
                    + '<a href="patches.py?%s&amp;RateOrder=%s%s">'
                    % (urlstr, title[0], dirstr)
                    + '<img src="images/arrow_%s.png" alt="%s"' % (arrow, alt)
                    + ' width=16 height=14 border=0></a>',)

    return ret
