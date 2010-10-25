#!/usr/bin/python
#
# search.py - Instrument search functions.
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
import cgi
import string
import MySQLdb

from PatchItem import PatchHelper
import InstDB

# Put a cap on the max number of results
max_results = 500
max_sub_matches = 5     # Max number of sub matching fields to display

# Scores for different matching information (should all be powers of 2)
patch_name_score   = 256     # Patch title or file name match
inst_name_score    = 128     # For each instrument name match
sample_name_score  = 64      # For each sample name match
patch_prop_score   = 32      # For each patch property match
inst_prop_score    = 8       # For each instrument property match
sample_prop_score  = 4       # For each sample property match


# Mark matching text in a string
def mark_match (str, match):
    lowstr = string.lower (str)
    match = string.lower (match)
    lastpos = 0
    newstr = ""
    i = string.find (lowstr, match)
    while i >= 0:
        if i > lastpos: newstr += cgi.escape (str[lastpos:i])
        newstr += "<b><font color=\"#00FF00\">" \
                  + cgi.escape (str[i:i+len (match)]) + "</font></b>"
        lastpos = i + len (match)
        i = string.find (lowstr, match, lastpos)

    newstr += cgi.escape (str[lastpos:])

    return newstr


# query for matching info and add to "matches" with a limit on # of entries
def query_matches (query, matches, matchstr):
    left = max_sub_matches - len (matches)
    if left <= 0: return matches

    count = InstDB.Cur.execute (query)
    if count == 0: return matches
    if count > left: count = left

    for i in range (0, count):
        row = InstDB.Cur.fetchone ()
        matches += ((cgi.escape (row['type']), cgi.escape (row['name']),
                     mark_match (InstDB.emailmunge (row['value']),matchstr)),)
    return matches


def action (req, fields):
    text = fields.get ("Text", "")
    offset = int (fields.get ("Offset", 0))
    count = int (fields.get ("Count", 20))

    exceeded = 0                        # set to 1 if max_results is exceeded

    try:
        InstDB.Cur.execute ("DROP TABLE _scores")
        InstDB.Cur.execute ("DROP TABLE _total_scores")
    except MySQLdb.OperationalError:
        pass

    # Create temporary scoring table
    InstDB.Cur.execute ("""\
CREATE TEMPORARY TABLE _scores (
pid INT UNSIGNED NOT NULL,
rating INT UNSIGNED NOT NULL,
flag SMALLINT UNSIGNED NOT NULL,
INDEX (pid))""")

    # Create temporary total scores table
    InstDB.Cur.execute ("""\
CREATE TEMPORARY TABLE _total_scores (
pid INT UNSIGNED NOT NULL,
rating INT UNSIGNED NOT NULL,
flags SMALLINT UNSIGNED NOT NULL,
INDEX (pid),
INDEX (rating))""")

    # Insert scores for matches to Patch Name or FileName
    rows = InstDB.Cur.execute ("""\
INSERT INTO _scores
SELECT PatchId, %s, %s FROM PatchInfo
WHERE INSTR(PatchName, %s) || INSTR(FileName, %s)
LIMIT %s""", (patch_name_score, patch_name_score, text, text, max_results))
    if rows == max_results: exceeded = 1

    # Insert scores for matches to Patch properties (each property ++)
    rows = InstDB.Cur.execute ("""\
INSERT INTO _scores
SELECT ItemProps.ItemId, COUNT(*) * %s, %s FROM ItemProps, Items
WHERE ItemProps.ItemId=Items.ItemId && Items.ItemType='Patch'
&& INSTR(PropValue, %s) GROUP BY ItemProps.ItemId LIMIT %s""",
                               (patch_prop_score, patch_prop_score,
                                text, max_results))
    if rows == max_results: exceeded = 1

    # Insert scores for matches to instrument names (each instrument ++)
    rows = InstDB.Cur.execute ("""\
INSERT INTO _scores
SELECT PatchId, COUNT(*) * %s, %s FROM InstInfo
WHERE INSTR(InstName, %s)
GROUP BY PatchId LIMIT %s""", (inst_name_score, inst_name_score,
                               text, max_results))
    if rows == max_results: exceeded = 1

    # Insert scores for matches to instrument properties (each property ++)
    rows = InstDB.Cur.execute ("""\
INSERT INTO _scores
SELECT InstInfo.PatchId, COUNT(*) * %s, %s FROM ItemProps, Items, InstInfo
WHERE ItemProps.ItemId=Items.ItemId && Items.ItemType='Inst'
&& INSTR(PropValue, %s) && ItemProps.ItemId=InstInfo.InstId
GROUP BY InstInfo.PatchId LIMIT %s""", (inst_prop_score, inst_prop_score,
                                        text, max_results))
    if rows == max_results: exceeded = 1

    # Insert scores for matches to sample names (each sample ++)
    rows = InstDB.Cur.execute ("""\
INSERT INTO _scores
SELECT PatchId, COUNT(*) * %s, %s FROM SampleInfo
WHERE INSTR(SampleName, %s)
GROUP BY PatchId LIMIT %s""", (sample_name_score, sample_name_score,
                               text, max_results))
    if rows == max_results: exceeded = 1

    # Insert scores for matches to sample properties (each property ++)
    rows = InstDB.Cur.execute ("""\
INSERT INTO _scores
SELECT SampleInfo.PatchId, COUNT(*) * %s, %s FROM ItemProps, Items, SampleInfo
WHERE ItemProps.ItemId=Items.ItemId && Items.ItemType='Sample'
&& INSTR(PropValue, %s) && ItemProps.ItemId=SampleInfo.SampleId
GROUP BY SampleInfo.PatchId LIMIT %s""", (sample_prop_score, sample_prop_score,
                                          text, max_results))
    if rows == max_results: exceeded = 1

    # Sum up all the ratings into temp total ratings table
    InstDB.Cur.execute ("""\
INSERT INTO _total_scores
SELECT pid, SUM(rating), BIT_OR(flag) FROM _scores, PatchInfo
WHERE pid = PatchInfo.PatchId && PatchInfo.State='Active'
GROUP BY pid""")

    # Drop temp ratings table
    InstDB.Cur.execute ("DROP TABLE _scores")

    # Get max rating
    InstDB.Cur.execute ("SELECT MAX(rating) AS max_rating,"
                        " COUNT(*) AS total_count FROM _total_scores")
    row = InstDB.Cur.fetchone ()
    max_rating = row["max_rating"]
    total_count = row["total_count"]

    itemCells = (("FileNameLink",),
                 ("PatchName",),
                 ("DownloadLink", "PatchType", "FileSize"),
                 ("Rating", "DateImported"),
                 ("CategoryName", "LicenseName"),
                 ("Version", "WebSiteLink", "Email"))


    # convert sequence of sequence of strings to list of string fields
    itemFields = []
    for cell in itemCells:
        for field in cell:
            itemFields += (field,)

    sel = PatchHelper.sql_select (itemFields) # Create SELECT query

    sel.where.insert (0, "PatchInfo.State = 'Active'")
    sel.where.insert (0, "_total_scores.pid=PatchInfo.PatchId")
    sel.tables += ("_total_scores",)
    sel.orderby = "rating DESC"
    sel.fields += ("pid", "rating", "flags")
    sel.limit = "%d, %d" % (offset, count)

    InstDB.Cur.execute (sel.query ())

    req.write (InstDB.header ("Search results"))

    if total_count == 0:
        req.write ("<b>No matches to '%s'</b>\n" % (cgi.escape (text)))
        req.write (InstDB.footer ())
        InstDB.Cur.execute ("DROP TABLE _total_scores")
        return

    if exceeded: warn = " (results truncated due to excessive matches)"
    else: warn = ""

    if total_count == 1:
        matchstr = "Found 1 match to '%s'%s\n" % (cgi.escape (text), warn)
    else: matchstr = "Found %d matches to '%s'%s\n" \
		% (total_count, cgi.escape (text), warn)

    req.write ("<b>" + matchstr + "</b>\n")

    # Re URL-ify the input fields
    urlstr = urlencode ((("Action", "search"), ("Text", text),
                         ("Count", count)))
    urlstr = cgi.escape (urlstr)

    # Display the result pager bar
    InstDB.show_pager (req, urlstr, offset, count, total_count)

    req.write ("<p>\n")

    # loop over matches
    for row in InstDB.Cur.fetchall ():

        score = int (float (row["rating"]) / float (max_rating) * 100.0)
        if score == 0: score = 1
        scoreStr = "Score %d (%s/%s)" % (score, row["rating"], max_rating)

        # box start for result item
        box = InstDB.html_box ()
        box.fontsize = "+0"
        box.tableattr = 'width="640"'
        req.write (box.start (scoreStr))

        tab = PatchHelper.cell_vals (itemCells, row)

        split = InstDB.tabsplits (tab)
        split.tableattr = 'width="100%"'
        req.write (split.render ())

        # Create array of matching info to display (Type, Name, Value)
        matches = ()
        flags = int (row["flags"])

        esctext = InstDB.sqlescape (text)

        if flags & patch_prop_score:    # Patch property matches?
            query = """\
SELECT '%s' AS type, P.PropName AS name, I.PropValue AS value
FROM ItemProps AS I, Props AS P
WHERE I.ItemId=%d && INSTR(I.PropValue, '%s') && I.PropId=P.PropId""" \
            % ("Patch Property", int (row["pid"]), esctext)
            matches = query_matches (query, matches, text)

        if flags & inst_name_score:     # Instrument name matches?
            query = "SELECT '%s' AS type, '%s' AS name, InstName AS value" \
                    " FROM InstInfo WHERE PatchId=%d && INSTR(InstName, '%s')" \
                    % ("Instrument", "Name", int (row["pid"]), esctext)
            matches = query_matches (query, matches, text)

        if flags & inst_prop_score:     # Instrument property matches?
            query = """\
SELECT '%s' AS type, Props.PropName AS name, ItemProps.PropValue AS value
FROM ItemProps, Props, Items, InstInfo
WHERE Items.ItemType='Inst' && INSTR(ItemProps.PropValue, '%s')
&& ItemProps.ItemId=InstInfo.InstId && InstInfo.PatchId=%d
&& ItemProps.ItemId=Items.ItemId && ItemProps.PropId=Props.PropId""" \
            % ("Instrument Property", esctext, int (row["pid"]))
            matches = query_matches (query, matches, text)

        if flags & sample_name_score:   # Sample name matches?
            query = "SELECT '%s' AS type, '%s' AS name, SampleName AS value" \
                    " FROM SampleInfo WHERE PatchId=%d" \
                    " && INSTR(SampleName, '%s')" \
                    % ("Sample", "Name", int (row["pid"]), esctext)
            matches = query_matches (query, matches, text)

        if flags & sample_prop_score:   # Sample property matches?
            query = """\
SELECT '%s' AS type, Props.PropName AS name, ItemProps.PropValue AS value
FROM ItemProps, Props, Items, SampleInfo
WHERE Items.ItemType='Sample' && INSTR(ItemProps.PropValue, '%s')
&& ItemProps.ItemId=SampleInfo.SampleId && SampleInfo.PatchId=%d
&& ItemProps.ItemId=Items.ItemId && ItemProps.PropId=Props.PropId""" \
            % ("Sample Property", esctext, int (row["pid"]))
            matches = query_matches (query, matches, text)

        if matches:
            table = InstDB.tabular (("Item", "Field", "Value"))
            table.tableattr = 'width="100%"'
            table.cellspacing = 2
            for m in matches:
                req.write (table.addrow (m))
            req.write (table.end ())

        req.write (box.end ())
        req.write ("<p>\n")

    req.write ("<p>\n")

    InstDB.show_pager (req, urlstr, offset, count, total_count)

    req.write (InstDB.footer ())

    InstDB.Cur.execute ("DROP TABLE _total_scores")
