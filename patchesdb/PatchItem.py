#!/usr/bin/python
#
# PatchItem.py - Helper classes for database tables and generating
# HTML or SQL commands.
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
import SqlHelpers
import MySQLdb
import InstDB

# Widths of rating star images (0.5, 1.0, 1.5, ..., 5.0)
StarWidths = (6,11,18,23,30,36,42,48,54,60)

def form_text (name, value, size=32):
    return "<input type=text name=%s value=\"%s\" size=%d>\n" \
        % (name, cgi.escape (value), size)

def form_textarea (name, value, cols=32, rows=3):
    return "<textarea name=%s cols=%d rows=%d>%s</textarea>\n" \
        % (name, cols, rows, cgi.escape (value))

def form_select (name, options, selected):
    s = "<select name=%s>\n" % name
    for i in range (0, len (options) & ~1, 2):
        if options[i] == selected: sel = " selected"
        else: sel = ""

        s += "<option value=%s%s> %s\n" % (options[i], sel, options[i + 1])

    s += "</select>\n"
    return s

def form_radio (name, options, selected):
    s = ""
    for i in range (0, len (options)):
        if options[i] == selected: sel = " checked"
        else: sel = ""

        s += "<input type=radio name=%s value=%s%s>%s\n" \
            % (name, options[i], sel, options[i])
    return s

def form_checkboxes (name, options, selected):
    s = ""
    for i in range (0, len (options) & ~1, 2):
        if options[i] in selected: sel = " checked"
        else: sel = ""

        s += "<input type=checkbox name=%s value=%s%s>%s<br>\n" \
            % (name, options[i], sel, options[i + 1])
    return s

class ItemHelper:
    "Item helper class"
    table = ""           # Table name
    fieldinfo = {}       # Field -> (Viewable, Editable, Orderable, Type, Title)

    # Viewable = True/False
    # Editable = True/False
    # Searchable/Orderable = 0/1/2: False/Ascending/Descending
    # Type char (Bool Integer Float String Enum flAgs List Date liNk eMail User Password)
    # Title - Name of field as shown to users

    list_format = ()      # Default list format sequence
    cell_format = ()      # Default cell format sequence of sequences
    edit_cell_format = () # Default edit cell format


    # Default SQL select field method (returns a sequence of SQL fields)
    def sql_fields (self, field):
        "Get SQL field for an item field"

        if not self.fieldinfo.get (field, None):
            return None

        return (self.table + "." + field,)

    # Default SQL table method
    # Returns a sequence of tables required for the given field
    def sql_tables (self, field):
        "Get SQL tables required for a field"

        return (self.table,)

    # Just a prototype, should be overridden by derived classes
    # Return: SQL join string for the given table
    def sql_table_join (self, table):
        "Get SQL join WHERE clause for a table"
        return None

    # Not all item fields are 1 to 1 in regards to searching/ordering
    def sql_search_field (self, field):
        "Get the SQL search/order field for an item field"

        # Get field config and make sure valid field name and searchable
        fieldcfg = self.fieldinfo.get (field, None)
        if not fieldcfg or not fieldcfg[2]: return None

        # Default to first field of sql_fields
        sqlfields = self.sql_fields (field)
        if not sqlfields: return None

        return sqlfields[0]

    # Allow field OrderBy overrides
    def sql_order_field (self, field, order=0):
        """Get the SQL orderable field for an item field.
        order: 0/1/2 - Default/Ascending/Descending"""

        searchfield = self.sql_search_field (field)
        if not searchfield: return None

        fieldcfg = self.fieldinfo[field]
        if order == 0: order = fieldcfg[2]

        # Sort order (2 for descending, defaults to ascending)
        if order == 2: return searchfield + ' DESC'
        else: return searchfield

    # Get SQL update field/values for a given item field and data
    # Returns a dictionary of SQL field name and SQL escaped value to assign
    def sql_update_fields (self, field, data):
        "Get SQL field name/values for a given item field and data"

        # Field is valid and editable?
        fieldcfg = self.fieldinfo.get (field, None)
        if not fieldcfg or not fieldcfg[1]: return None

        # Convert multi valued list to comma-delimited string (flags)
        val = data.get (field, "")
        if isinstance (val, list):
            val = string.join (val, ',')

        return { field : '"'+MySQLdb.escape_string (val)+'"' }

    # Default HTML value method
    def html_val (self, field, data):
        "Get HTML value for item field"

        # Get field config and make sure valid field name
        fieldcfg = self.fieldinfo.get (field, None)
        if not fieldcfg: return None

        # Field in data list?
        val = data.get (field, None)
        if val == None: return None

        return cgi.escape (str (val))

    def form_val (self, field, data, prefix=""):
        "Get HTML form edit value for item field"

        # Get field config and make sure valid field name and editable
        fieldcfg = self.fieldinfo.get (field, None)
        if not fieldcfg or not fieldcfg[1]: return None

	if data: value = data.get (field, "")
	else: value = ""

        type = fieldcfg[3]

        if type == 'B':                 # Boolean
            if value: checked = " checked"
            else: checked = ""
            return "<input type=checkbox name=\"%s%s\"%s>" % (prefix, field,
                                                              checked)
        elif type in ('I', 'F'):
            return "<input type=text name=\"%s%s\" value=\"%s\" size=10" \
                   " maxlength=10>" % (prefix, field, cgi.escape (str (value)))
        else:                           # Default to string field
            return "<input type=text name=\"%s%s\" value=\"%s\">" \
                   % (prefix, field, cgi.escape (value))


    def field_viewable (self, field):
        "Check if field is viewable"
        fieldcfg = self.fieldinfo.get (field, None)
        if not fieldcfg: return False

        return fieldcfg[0]

    def field_editable (self, field):
        "Check if field is editable"
        fieldcfg = self.fieldinfo.get (field, None)
        if not fieldcfg: return False

        return fieldcfg[1]

    def field_searchable (self, field):
        """Check if field is searchable/orderable
Returns: 0/1/2 - False/Ascending/Descending"""

        fieldcfg = self.fieldinfo.get (field, None)
        if not fieldcfg: return False

        return fieldcfg[2]

    def field_title (self, field):
        "Get the title for a field"

        fieldcfg = self.fieldinfo.get (field, None)
        if not fieldcfg: return None

        return fieldcfg[4]



    # Don't normally need to override these


    # Create an SQL select query for the given fields
    def sql_select (self, fields):
        "Create an SQL select helper for the given item fields"

        sqlfields = {}
        sqltables = { self.table : 1 }

        for f in fields:
            sqlf = self.sql_fields (f)
            if not sqlf:
                raise LookupError, "SQL field '%s' not found" % f

            # Add fields to dictionary to weed out duplicates
            sqlfields.update ({}.fromkeys (sqlf, 1))

            sqlt = self.sql_tables (f)
            if sqlt:       # Add tables to dictionary to weed out duplicates
                sqltables.update ({}.fromkeys (sqlt, 1))

        sel = SqlHelpers.Select ()
        sel.fields = sqlfields.keys ()
        sel.tables = sqltables.keys ()
        sel.where = []

        # Add table join WHERE clauses if any
        for t in sqltables.keys ():
            if t != self.table:
                join = self.sql_table_join (t)
                if join:
                    sel.where.append (join)

        return sel

    # Create an SQL update query for the given fields and data values
    def sql_update (self, fields, data):

        update = SqlHelpers.Update ()
        update.table = self.table
        update.where = "invalid='OMFG!'" # Make sure where gets set..

        for field in fields:
            if self.fieldinfo[field][1]:
                val = self.sql_update_fields (field, data)
                if val: update.setnesc.update (val)

        return update

    # Create an SQL insert query for the given fields and data values
    def sql_insert (self, fields, data):

        insert = SqlHelpers.Insert ()
        insert.table = self.table

        for field in fields:
            if self.fieldinfo[field][1]:
                val = self.sql_update_fields (field, data)
                if val: insert.setnesc.update (val)

        return insert

    # Gets values for a list of fields from the given data row
    # Returns: List of HTML field values
    def html_vals (self, fields, data):
        "Get HTML values for item fields"

        vals = []
        for f in fields:
            vals.append (self.html_val (f, data))
        return vals

    # Gets form values for a list of fields from the given data row
    # View only fields will be returned as non-form values
    # Returns: List of HTML form field values
    def form_vals (self, fields, data, prefix=""):
        "Get HTML form values for item fields"

        vals = []
        for field in fields:
            fieldcfg = self.fieldinfo[field]

            if not fieldcfg: vals.append (None)
            elif fieldcfg[1]: vals.append (self.form_val (field, data, prefix))
            else: vals.append (self.html_val (field, data))

        return vals

    # Returns title/value cells for given field cells (sequence of sequences)
    def cell_vals (self, cells, data):
        "Get HTML title/value cells for field cells"

        tab = []
        for cell in cells:
            tabcell = []
            for field in cell:
                title = self.field_title (field)
                value = self.html_val (field, data)
                if title and value != "":
                    tabcell += (title, value)

            if tabcell: tab += (tabcell,)

        return tab

    # Returns title/value form cells for given field cells
    def form_cell_vals (self, cells, data, prefix=""):
        "Get HTML title/form value cells for field cells"

        tab = []
        for cell in cells:
            tabcell = []
            for field in cell:
                title = self.field_title (field)

                fieldcfg = self.fieldinfo[field]
                if fieldcfg[1]: value = self.form_val (field, data, prefix)
                else: value = self.html_val (field, data)

                if title and value != "":
                    tabcell += (title, value)

            if tabcell: tab += (tabcell,)

        return tab


class PatchHelperClass(ItemHelper):
    "Patch item helper class"

    table = "PatchInfo"

    # Field : (Viewable, Editable, Order (0=None/1=ASC/2=DESC), Type, Title)
    fieldinfo = {
        "PatchId" :       (True,  False, 1, 'I', "ID"),
        "PatchFlags" :    (False, False, 0, 'A', "Flags"),
        "State" :         (True,  False, 1, 'E', "State"),
        "UserId" :        (False, False, 0, 'U', "User ID"),
        "PatchAuthor" :   (True,  True,  1, 'S', "Author"),
        "FileName" :      (True,  False, 1, 'S', "File Name"),
        "PatchType" :     (True,  False, 1, 'E', "Type"),
        "FileSize" :      (True,  False, 2, 'I', "File Size"),
        "PatchName" :     (True,  True,  1, 'S', "Name"),
        "PatchDescr" :    (True,  True,  1, 'S', "Description"),
        "CramSize" :      (True,  False, 2, 'I', "CRAM Size"),
        "CramClicks" :    (True,  False, 2, 'I', "CRAM DLs"),
        "ZipSize" :       (True,  False, 2, 'I', "ZIP Size"),
        "ZipClicks" :     (True,  False, 2, 'I', "ZIP DLs"),
        "InfoClicks" :    (True,  False, 2, 'I', "Views"),
        "CategoryId":     (True,  True,  1, 'L', "Category"),
        "CategoryId2":    (True,  True,  1, 'L', "Category2"),
        "CategoryId3":    (True,  True,  1, 'L', "Category3"),
        "LicenseId" :     (True,  True,  1, 'L', "License"),
        "Version" :       (True,  True,  1, 'S', "Version"),
        "WebSite" :       (True,  True,  1, 'N', "Website"),
        "Email" :         (True,  True,  1, 'M', "Email"),
        "RateTotal" :     (False, False, 0, 'I', "Rating Sum"),
        "RateCount" :     (True,  False, 2, 'I', "Ratings"),
        "UserModified" :  (True,  False, 1, 'U', "User Modified"),
        "UserImported" :  (True,  False, 1, 'U', "User Imported"),
        "DateModified" :  (True,  False, 2, 'D', "Date Modified"),
        "DateImported" :  (True,  False, 2, 'D', "Date Imported"),

        # Fields not in PatchInfo table
        "Author" :        (True,  False, 1, 'S', "Author"),
        "CategoryName" :  (True,  False, 1, 'S', "Category"),
        "CategoryName2" : (True,  False, 0, 'S', "Category"),
        "CategoryName3" : (True,  False, 0, 'S', "Category"),
        "CategoryLong" :  (True,  False, 1, 'S', "Category"),
        "Date" :          (True,  False, 2, 'D', "Date"),
        "DownloadClicks" :(True,  False, 2, 'I', "Downloads"),
        "DownloadLink" :  (True,  False, 2, 'N', "Download"),
        "FileNameLink" :  (True,  False, 1, 'N', "File Name"),
        "CramSizeDetail" :(True,  False, 0, 'S', "CRAM Size"),
        "ZipSizeDetail" : (True,  False, 0, 'S', "ZIP Size"),
        "IsAuthor" :      (True,  True,  1, 'B', "Are Author?"),
        "LicenseName" :   (True,  False, 1, 'S', "License"),
        "LicenseLink" :   (True,  False, 1, 'N', "License"),
        "Rating" :        (True,  False, 2, 'N', "Rating"),
        "RatingValue" :   (True,  False, 2, 'F', "Rating"),
        "UserImportedName":(True, False, 1, 'S', "Imported by"),
        "UserLink" :      (True,  False, 1, 'N', "Owner"),
        "UserModifiedName":(True, False, 1, 'S', "Modified by"),
        "UserName" :      (True,  False, 1, 'S', "Owner"),
        "WebSiteLink" :   (True,  False, 1, 'N', "Website"),
        }

    # Default list format
    list_format = ("PatchType", "FileNameLink", "CategoryLong",
                   "LicenseLink", "FileSize", "DownloadLink",
                   "Rating", "Date")

    # Default cell format
    cell_format = (("FileName",),
                   ("PatchName",),
                   ("Author",),
                   ("PatchType", "Rating"),
                   ("DownloadLink", "ZipSizeDetail", "FileSize"),
                   ("UserImportedName", "DateImported"),
                   ("CategoryLong",),
                   ("LicenseLink",),
                   ("Version", "WebSiteLink", "Email"),
                   ("PatchDescr",))

    # Default edit cell format
    edit_cell_format = (("FileName",),
                        ("PatchName",),
                        ("IsAuthor", "PatchAuthor"),
                        ("Version",),
                        ("CategoryId",),
                        ("CategoryId2",),
                        ("CategoryId3",),
                        ("LicenseId",),
                        ("PatchDescr",),
                        ("WebSite", "Email"))

    # Cached hash of categories { id => name }
    Categories = {}

    # Get a category name by ID (caches the category list)
    def category_name (self, id):
        if not self.Categories:
            InstDB.Cur.execute ("SELECT CategoryId, CategoryName FROM Category")
            for row in InstDB.Cur.fetchall ():
                self.Categories[int (row["CategoryId"])] = row["CategoryName"]
        return self.Categories.get (id, None)

    def sql_fields (self, field):
        if not self.fieldinfo.get (field, None): return None

        if field == "Author": return ("PatchInfo.PatchFlags", "Users.UserName",
                                      "PatchInfo.PatchAuthor")
        elif field == "CategoryName": return ("Category.CategoryName",)
        elif field == "CategoryName2": return ("PatchInfo.CategoryId2",)
        elif field == "CategoryName3": return ("PatchInfo.CategoryId3",)
        elif field == "CategoryLong":
            return ("Category.CategoryName", "PatchInfo.CategoryId",
                    "PatchInfo.CategoryId2", "PatchInfo.CategoryId3")
        elif field == "CramSizeDetail": return ("PatchInfo.CramSize",)
        elif field == "Date":
            return ("DATE_FORMAT(PatchInfo.DateImported,'%Y-%m-%d') AS Date",)
        elif field == "DownloadClicks":
            return ("PatchInfo.CramClicks + PatchInfo.ZipClicks AS DownloadClicks",)
        elif field == "DownloadLink":
            return ("PatchInfo.CramClicks + PatchInfo.ZipClicks AS DownloadClicks",
                    "PatchInfo.PatchId")
        elif field == "FileName":
            return ("PatchInfo.FileName", "PatchInfo.PatchType")
        elif field == "FileNameLink":
            return ("PatchInfo.FileName", "PatchInfo.PatchId",
                    "PatchInfo.PatchType")
        elif field == "IsAuthor":
            return ("FIND_IN_SET('Author', PatchInfo.PatchFlags) AS IsAuthor",)
        elif field == "LicenseName": return ("Licenses.LicenseName",)
        elif field == "LicenseLink":
            return ("Licenses.LicenseName", "Licenses.LicenseURL")
        elif field in ("Rating", "RatingValue"):
            return ("IF(PatchInfo.RateCount, PatchInfo.RateTotal /"
                    " PatchInfo.RateCount, 0) AS RatingValue",
                    "PatchInfo.RateCount")
        elif field == "UserImportedName":
            return ("Users2.UserName AS UserImportedName",)
        elif field == "UserLink": return ("Users.UserName", "Users.UserId")
        elif field == "UserModifiedName":
            return ("Users3.UserName AS UserModifiedName",)
        elif field == "UserName": return ("Users.UserName",)
        elif field == "ZipSizeDetail": return ("PatchInfo.ZipSize",)
        elif field == "WebSiteLink": return ("PatchInfo.WebSite",)
        else:
            return ItemHelper.sql_fields (self, field)

    def sql_tables (self, field):
        if not self.fieldinfo.get (field, None):
            return None

        if field == "Author": return ("Users",)
        elif field in ("CategoryName", "CategoryLong"): return ("Category",)
        elif field in ("LicenseName", "LicenseLink"): return ("Licenses",)
        elif field in ("UserLink", "UserName"): return ("Users",)
        elif field == "UserImportedName": return ("Users AS Users2",)
        elif field == "UserModifiedName": return ("Users AS Users3",)
        else:
            return ItemHelper.sql_tables (self, field)

    def sql_table_join (self, table):
        if table == "Category":
            return "PatchInfo.CategoryId=Category.CategoryId"
        elif table == "Licenses":
            return "PatchInfo.LicenseId=Licenses.LicenseId"
        elif table == "Users":
            return "PatchInfo.UserId=Users.UserId"
        elif table == "Users AS Users2":
            return "PatchInfo.UserImported=Users2.UserId"
        elif table == "Users AS Users3":
            return "PatchInfo.UserModified=Users3.UserId"
        else: return None

    def sql_order_field (self, field, order=0):
        if field == "CategoryLong":
            if order == 2: orderstr = " DESC"
            else: orderstr = ""
            return "CategoryName%s" % orderstr
        elif field == "DownloadLink":
            if order in (0, 2): orderstr = " DESC"
            else: orderstr = ""
            return "DownloadClicks%s" % orderstr
        elif field == "FileName":
            if order == 2: orderstr = " DESC"
            else: orderstr = ""
            return "FileName%s" % orderstr
        elif field == "LicenseLink":
            if order == 2: orderstr = " DESC"
            else: orderstr = ""
            return "LicenseName%s" % orderstr
        elif field in ("Rating", "RatingValue"):
            if order in (0, 2): orderstr = " DESC"
            else: orderstr = ""
            return "RatingValue%s, RateCount%s, FileName" % (orderstr, orderstr)
        elif field in ("Date", "DateImported"):
            if order in (0, 2): orderstr = " DESC"
            else: orderstr = ""
            return "DateImported%s, FileName" % orderstr
        else:
            orderfield = ItemHelper.sql_order_field (self, field, order)

            # FileName is secondary orderby field
            if orderfield: return orderfield + ", FileName"
            else: return None

    def sql_update_fields (self, field, data):
        if field in ("CategoryId", "CategoryId2", "CategoryId3"):
            id = int (data[field])

            # Check if valid category (0 allowed for CategoryId 2 and 3)
            if (id == 0 and field != "CategoryId") \
                   or self.category_name (id):
                return { field : str (id) }

            # Set it to the default value if invalid
            elif field == "CategoryId": return { field : str (1) }
            else: return { field : str (0) }
        elif field == "LicenseId":
            val = int (data[field])
            InstDB.Cur.execute ("SELECT COUNT(*) AS count FROM Licenses"
                                " WHERE LicenseId=%s", val)
            count = InstDB.Cur.fetchone ()["count"]
            if count == 1:
                return { field : str (val) }
            else: return { field : str (1) }   # Set to default val if invalid
        elif field == "IsAuthor":
            if data.get ("IsAuthor", None):
                return { "PatchFlags" : "PatchFlags | 1" }
            else: return { "PatchFlags" : "PatchFlags & ~1" }
        else:
            return ItemHelper.sql_update_fields (self, field, data)

    def html_val (self, field, data):
        global StarWidths

        if field == "Author":
            if "Author" in data["PatchFlags"]:
                return cgi.escape (InstDB.emailmunge (data["UserName"]))
            else: return cgi.escape (InstDB.emailmunge (data["PatchAuthor"]))
        elif field == "CategoryName2":
            return self.category_name (int (data["CategoryId2"]))
        elif field == "CategoryName3":
            return self.category_name (int (data["CategoryId3"]))
        elif field == "CategoryLong":
            cats = data["CategoryName"]
            if data["CategoryId"] != 1:
                s = self.category_name (int (data["CategoryId2"]))
                if s:
                    cats = cats + ", " + s
                    s = self.category_name (int (data["CategoryId3"]))
                    if s:
                        cats = cats + ", " + s
            return cats
        elif field == "CramSize":
            return InstDB.pretty_size (data["CramSize"])
        elif field == "CramSizeDetail":
            return InstDB.pretty_size (data["CramSize"]) \
                   + " (%d)" % data["CramSize"]
        elif field == "DownloadLink":
            return "<a href=\"download.py?PatchId=%d&amp;Type=zip\">" \
                   "<img src=\"images/dnld.png\" alt=\"Download\" width=14 height=14 border=0>" \
                   "</a> (%d)" % (data["PatchId"], data["DownloadClicks"])
        elif field == "Email":
            return cgi.escape (InstDB.emailmunge (data["Email"]))
        elif field == "FileName":
            return cgi.escape (data["FileName"]) + "." + data["PatchType"]
        elif field == "FileNameLink":
            return '<a href="patches.py?Action=item&amp;ItemId=%d">%s.%s</a>' \
                   % (data["PatchId"], cgi.escape (data["FileName"]),
                      data["PatchType"])
        elif field == "FileSize":
            return InstDB.pretty_size (data["FileSize"])
        elif field == "LicenseLink":
            if data["LicenseURL"]:
                return '<a href="%s">%s</a>' % (data["LicenseURL"],
                                                data["LicenseName"])
            else: return data["LicenseName"]
        elif field == "Rating":
            if data["RateCount"] > 0:
                f = float (data["RatingValue"])
                if f < 0.5: f = 0.5
                elif f > 5.0: f = 5.0

                fract = f - int (f)
                if fract < 0.5: ifract = 0
                else: ifract = 5

                width = StarWidths[int(f)*2 + ifract/5 - 1]

                return ('<img src="images/stars_%d_%d.png" alt="%0.1f stars" width=%d height=11>'
                        '(%d)' % (int (f), ifract, f, width, data["RateCount"]))
            else: return "None"
        elif field == "UserFileSize":
            return InstDB.pretty_size (data["FileSize"])
        elif field == "UserLink":
            return "<a href=\"patches.py?Action=user&amp;UserId=%d\">%s</a>" \
                   % (data["UserId"], cgi.escape (data["UserName"]))
        elif field == "ZipSize":
            return InstDB.pretty_size (data["ZipSize"])
        elif field == "ZipSizeDetail":
            return InstDB.pretty_size (data["ZipSize"]) \
                   + " (%d)" % data["ZipSize"]
        elif field == "WebSiteLink":
            escurl = cgi.escape (data["WebSite"])
            return '<a href="%s">%s</a>' % (escurl, escurl)
        else:
            return ItemHelper.html_val (self, field, data)


    def form_val (self, field, data, prefix=""):
        if not self.fieldinfo.get (field, None):
            return None

        if field == "LicenseId":
            InstDB.Cur.execute ("SELECT LicenseId, LicenseName FROM Licenses"
                                " ORDER BY LicenseId")
            rows = InstDB.Cur.fetchall ()

            options = []
            for row in rows:
                options += (row["LicenseId"], row["LicenseName"])

            return form_select (prefix+"LicenseId", options, data["LicenseId"])
        elif field in ("CategoryId", "CategoryId2", "CategoryId3"):
            InstDB.Cur.execute ("SELECT CategoryId, CategoryName FROM Category"
                                " ORDER BY CategoryName")
            rows = InstDB.Cur.fetchall ()

            if field != "CategoryId": options = ("0", "-- Not Selected --")
            else: options = []

            for row in rows:
                options += (row["CategoryId"], row["CategoryName"])

            return form_select (prefix+field, options, data[field])
        elif field in ("PatchName", "FileName"):
            return form_text (prefix+field, data[field], 64)
        elif field == "PatchDescr":
            return form_textarea (prefix+field, data[field], 64, 4)
        else:
            return ItemHelper.form_val (self, field, data, prefix)


class InstHelperClass(ItemHelper):
    "Instrument item helper class"

    table = "InstInfo"

    # Field : (Viewable, Editable, Order (0=None/1=ASC/2=DESC), Type, Title)
    fieldinfo = {
        "InstId" :        (True,  False, 1, 'I', "ID"),
        "PatchId" :       (True,  False, 1, 'I', "Patch ID"),
        "Bank" :          (True,  False, 1, 'I', "Bank"),
        "Program" :       (True,  False, 0, 'I', "Program"),
        "InstName" :      (True,  False, 1, 'I', "Name"),

        # Fields not in InstInfo table
        "InstNameLink" :  (True,  False, 1, 'N', "Name")
        }

    list_format = ("Bank", "Program", "InstName")

    def sql_fields (self, field):
        if self.fieldinfo.get (field, None): pass
        elif PatchHelper.fieldinfo.get (field, None):
            return PatchHelper.sql_fields (field)
        else: return None

        if field == "InstNameLink":
            return ("InstInfo.InstName", "InstInfo.InstId")
        else:
            return ItemHelper.sql_fields (self, field)

    def sql_tables (self, field):
        if self.fieldinfo.get (field, None): pass
        elif PatchHelper.fieldinfo.get (field, None):
            return PatchHelper.sql_tables (field)
        else: return None

        if field == "State": return ("PatchInfo",)
        else:
            return ItemHelper.sql_tables (self, field)

    def sql_table_join (self, table):
        if table == "PatchInfo":
            return "InstInfo.PatchId=PatchInfo.PatchId"
        else:
            return PatchHelper.sql_table_join (table)

    def sql_search_field (self, field):
        if self.fieldinfo.get (field, None):
            return ItemHelper.sql_search_field (self, field)
        else:
            return PatchHelper.sql_search_field (field)

    def sql_order_field (self, field, order=0):
        if field == "Bank":
            if order == 2: orderstr = " DESC"
            else: orderstr = ""
            return "InstInfo.Bank%s, InstInfo.Program%s" % (orderstr, orderstr)
        elif field == "InstName":
            if order == 2: orderstr = " DESC"
            else: orderstr = ""
            return "InstInfo.InstName%s" % orderstr
        elif self.fieldinfo.get (field, None):
            orderfield = ItemHelper.sql_order_field (self, field, order)
        else:
            orderfield = PatchHelper.sql_order_field (field, order)

        # InstName is secondary order field
        if orderfield: return orderfield + ", InstInfo.InstName"
        else: return None

    def html_val (self, field, data):
        if self.fieldinfo.get (field, None): pass
        elif PatchHelper.fieldinfo.get (field, None):
            return PatchHelper.html_val (field, data)
        else: return None

        if field == "InstNameLink":
            return "<a href=\"patches.py?Action=item&amp;ItemId=%d\">%s</a>" \
                   % (data["InstId"], cgi.escape (data["InstName"]))
        else:
            return ItemHelper.html_val (self, field, data)

    def field_viewable (self, field):
        fieldcfg = self.fieldinfo.get (field, None)
        if not fieldcfg: fieldcfg = PatchHelper.fieldinfo.get (field, None)
        if not fieldcfg: return None

        return fieldcfg[0]

    def field_editable (self, field):
        fieldcfg = PatchHelper.fieldinfo.get (field, None)
        if not fieldcfg: return None

        return fieldcfg[1]

    def field_searchable (self, field):
        fieldcfg = self.fieldinfo.get (field, None)
        if not fieldcfg: fieldcfg = PatchHelper.fieldinfo.get (field, None)
        if not fieldcfg: return None

        return fieldcfg[2]

    def field_title (self, field):
        fieldcfg = self.fieldinfo.get (field, None)
        if not fieldcfg: fieldcfg = PatchHelper.fieldinfo.get (field, None)
        if not fieldcfg: return None

        return fieldcfg[4]


class SampleHelperClass(ItemHelper):
    "Sample item helper class"

    table = "SampleInfo"

    # Field : (Viewable, Editable, Order (0=None/1=ASC/2=DESC), Type, Title)
    fieldinfo = {
        "SampleId" :        (True,  False, 1, 'I', "ID"),
        "PatchId" :         (True,  False, 1, 'I', "Patch ID"),
        "Format" :          (True,  False, 1, 'E', "Format"),
        "SampleSize" :      (True,  False, 2, 'I', "Size"),
        "Channels" :        (True,  False, 2, 'I', "Channels"),
        "RootNote" :        (True,  False, 1, 'I', "Root Note"),
        "SampleRate" :      (True,  False, 2, 'I', "Sample Rate"),
        "SampleName" :      (True,  False, 1, 'S', "Name"),

        # Not in database
        "FormatStr" :       (True,  False, 0, 'S', "Format"),
        "SampleNameLink" :  (True,  False, 1, 'N', "Name")
        }

    list_format = ("SampleName", "SampleSize", "FormatStr", "RootNote")

    def sql_fields (self, field):
        if self.fieldinfo.get (field, None): pass
        elif PatchHelper.fieldinfo.get (field, None):
            return PatchHelper.sql_fields (field)
        else: return None

        if field == "CategoryName": return ("Category.CategoryName",)
        elif field == "FormatStr":
            return ("SampleInfo.Format", "SampleInfo.Channels",
                    "SampleInfo.SampleRate")
        elif field == "SampleNameLink":
            return ("SampleInfo.SampleName", "SampleInfo.SampleId")
        else:
            return ItemHelper.sql_fields (self, field)

    def sql_tables (self, field):
        if self.fieldinfo.get (field, None): pass
        elif PatchHelper.fieldinfo.get (field, None):
            return PatchHelper.sql_tables (field)
        else: return None

        if field == "CategoryName": return ("Category",)
        else:
            return ItemHelper.sql_tables (self, field)

    def sql_table_join (self, table):
        if table == "PatchInfo":
            return "SampleInfo.PatchId=PatchInfo.PatchId"
        else:
            return PatchHelper.sql_table_join (table)

    def sql_search_field (self, field):
        if self.fieldinfo.get (field, None):
            return ItemHelper.sql_search_field (self, field)
        else:
            return PatchHelper.sql_search_field (field)

    def sql_order_field (self, field, order=0):
        if field == "SampleName":
            if order == 2: orderstr = " DESC"
            else: orderstr = ""
            return "SampleInfo.SampleName%s" % orderstr
        elif self.fieldinfo.get (field, None):
            orderfield = ItemHelper.sql_order_field (self, field, order)
        else:
            orderfield = PatchHelper.sql_order_field (field, order)

        # SampleName is secondary order field
        if orderfield: return orderfield + ", SampleInfo.SampleName"
        else: return None

    def html_val (self, field, data):
        if self.fieldinfo.get (field, None): pass
        elif PatchHelper.fieldinfo.get (field, None):
            return PatchHelper.html_val (field, data)
        else: return None

        if field == "FormatStr":
            format = "%.1fkHz" % (int (data["SampleRate"]) / 1000.0)

            chan = int (data["Channels"])
            if chan == 1: format += " Mono"
            elif chan == 2: format += " Stereo"
            else: format += " %d-channel" % chan

            format += " " + data["Format"]

            return format

        elif field == "SampleNameLink":
            return "<a href=\"patches.py?Action=item&amp;ItemId=%d\">%s</a>" \
                   % (data["SampleId"], cgi.escape (data["SampleName"]))
        elif field == "SampleSize":
            return InstDB.pretty_size (data["SampleSize"])
        else:
            return ItemHelper.html_val (self, field, data)

    def field_viewable (self, field):
        fieldcfg = self.fieldinfo.get (field, None)
        if not fieldcfg: fieldcfg = PatchHelper.fieldinfo.get (field, None)
        if not fieldcfg: return None

        return fieldcfg[0]

    def field_editable (self, field):
        fieldcfg = PatchHelper.fieldinfo.get (field, None)
        if not fieldcfg: return None

        return fieldcfg[1]

    def field_searchable (self, field):
        fieldcfg = self.fieldinfo.get (field, None)
        if not fieldcfg: fieldcfg = PatchHelper.fieldinfo.get (field, None)
        if not fieldcfg: return None

        return fieldcfg[2]

    def field_title (self, field):
        fieldcfg = self.fieldinfo.get (field, None)
        if not fieldcfg: fieldcfg = PatchHelper.fieldinfo.get (field, None)
        if not fieldcfg: return None

        return fieldcfg[4]


class PropHelperClass(ItemHelper):
    "Item properties helper class"

    table = "ItemProps"

    # Field : (Viewable, Editable, Order (0=None/1=ASC/2=DESC), Type, Title)
    fieldinfo = {
        "ItemId" :       (True,  False, 1, 'I', "Item ID"),
        "PropId" :       (True,  False, 1, 'I', "Property ID"),
        "PropValue" :    (True,  True,  1, 'S', "Value"),

        # Fields not in ItemProps table
        "PropName" :     (True,  False, 1, 'S', "Name"),
        "PropDescr" :    (True,  False, 1, 'S', "Description")
        }

    list_format = ("PropName", "PropValue")
    cell_format = (("PropName", "PropValue"),)

    def sql_fields (self, field):
        if not self.fieldinfo.get (field, None): return None

        if field == "PropName":
            return ("Props.PropName",)
        elif field == "PropDescr":
            return ("Props.PropDescr",)
        else:
            return ItemHelper.sql_fields (self, field)

    def sql_tables (self, field):
        if not self.fieldinfo.get (field, None): return None

        if field in ("PropName", "PropDescr"): return ("Props",)
        else:
            return ItemHelper.sql_tables (self, field)

    def sql_table_join (self, table):
        if table == "Props":
            return "ItemProps.PropId=Props.PropId"
        else: return None

    def html_val (self, field, data):
        if not self.fieldinfo.get (field, None): return None

        if field == "PropValue":
            return cgi.escape (InstDB.emailmunge (data["PropValue"]))
        else:
            return ItemHelper.html_val (self, field, data)


class RatingHelperClass(ItemHelper):
    "Ratings/comments helper class"

    table = "Ratings"

    # Field : (Viewable, Editable, Order (0=None/1=ASC/2=DESC), Type, Title)
    fieldinfo = {
        "PatchId" :       (True,  False, 1, 'I', "Patch ID"),
        "UserId" :        (False, False, 0, 'U', "User ID"),
        "Date" :          (True,  False, 2, 'D', "Date"),
        "Rating" :        (True,  True,  2, 'I', "Rating"),
        "Comment" :       (True,  True,  0, 'S', "Comment"),

        # Fields not in InstInfo table
        "UserLink" :      (True,  False, 1, 'N', "User"),
        "UserName" :      (True,  False, 1, 'S', "User")
        }

    list_format = ("Date", "UserLink", "Rating", "Comment")

    def sql_fields (self, field):
        if not self.fieldinfo.get (field, None): return None

        if field == "UserLink":
            return ("Users.UserName", "Users.UserId")
        elif field == "UserName":
            return ("Users.UserName",)
        else:
            return ItemHelper.sql_fields (self, field)

    def sql_tables (self, field):
        if not self.fieldinfo.get (field, None): return None

        if field in ("UserLink", "UserName"): return ("Users",)
        else:
            return ItemHelper.sql_tables (self, field)

    def sql_table_join (self, table):
        if table == "Users":
            return "Ratings.UserId=Users.UserId"
        else: return None

    def html_val (self, field, data):
        global StarWidths

        if not self.fieldinfo.get (field, None): return None

        if field == "UserLink":
            return "<a href=\"patches.py?Action=user&amp;UserId=%d\">%s</a>" \
                   % (data["UserId"], cgi.escape (data["UserName"]))
        elif field == "Rating":
            rating = int (data["Rating"])
            if rating > 0:
                if rating > 5: rating = 5
                return ('<img src="images/stars_%d_0.png" alt="%d stars" width=%d height=11>'
                        % (rating, rating, StarWidths[rating * 2 - 1]))
            else: return "None"
        else:
            return ItemHelper.html_val (self, field, data)


class UsersHelperClass(ItemHelper):
    "Users table helper class"

    table = "Users"

    # Field : (Viewable, Editable, Order (0=None/1=ASC/2=DESC), Type, Title)
    fieldinfo = {
        "UserFlags" :    (True,  True,  0, 'A', "Options"),
        "Login" :        (True,  True,  1, 'S', "Login"),
        "UserName" :     (True,  True,  1, 'S', "User name"),
        "RealName" :     (True,  True,  1, 'S', "Real name"),
        "UserEmail" :    (True,  True,  1, 'S', "Email"),
        "WebSite" :      (True,  True,  1, 'S', "Website"),
        "WebSiteLink" :  (True,  False, 1, 'N', "Website"),
        "EmailVisible" : (True,  True,  0, 'E', "Email visible"),
        "RealNameVisible":(True, True,  0, 'E', "Real name visible"),
        "LastLogin" :    (True,  False, 2, 'D', "Last login"),
        "DateCreated" :  (True,  False, 2, 'D', "Created")
        }

    # Flag descriptions for UserFlags
    UserFlagOps = (
        "EmailNews", "Email me news",
        "NotifyRatings", "Notify on rating")

    list_format = ("Name", "WebSiteLink", "DateCreated")
    cell_format = (("Login", ""),)
    edit_cell_format = (
        ("UserEmail",),
        ("UserName",),
        ("RealName",),
        ("WebSite",),
        ("EmailVisible",),
        ("RealNameVisible",),
        ("UserFlags",))

    def form_val (self, field, data, prefix=""):
        if not self.fieldinfo.get (field, None):
            return None

        if field in ("RealNameVisible", "EmailVisible"):
            if data: val = data.get (field, "")
            else: val = ""

            if not val:
                if field == "EmailVisible": val = "No"
                else: val = "All"

            return form_radio (prefix + field, ("No", "Users", "All"), val)
        elif field == "UserFlags":
            if data: val = data.get (field, "")

            return form_checkboxes (prefix + field, self.UserFlagOps, val)
        else:
            return ItemHelper.form_val (self, field, data, prefix)

    def sql_fields (self, field):
        if not self.fieldinfo.get (field, None): return None

        if field == "WebSiteLink":
            return ("Users.WebSite",)
        else:
            return ItemHelper.sql_fields (self, field)

    def html_val (self, field, data):
        if not self.fieldinfo.get (field, None): return None

        if field == "WebSiteLink":
            escurl = cgi.escape (data["WebSite"])
            return '<a href="%s">%s</a>' % (escurl, escurl)
        else:
            return ItemHelper.html_val (self, field, data)

def props_select (itemId):
    "Create an SQL select object to get ItemProps for the given item ID"

    sel = SqlHelpers.Select ()
    sel.fields = ["Props.PropId", "Props.PropName", "ItemProps.PropValue"]
    sel.tables = ["ItemProps", "Props"]
    sel.where = ["ItemProps.ItemId=%d" % int (itemId),
                 "ItemProps.PropId=Props.PropId"]
    return sel

def props_form_cells (proprows, prefix=""):
    "Create HTML form table cells for item properties"

    # Add all the properties to a hash
    propvals = {}
    for prop in proprows:
        propvals[string.capitalize (prop["PropName"])] \
            = { "Id":prop["PropId"], "Value":cgi.escape (prop["PropValue"]) }

    # Sort the property name keys
    sortkeys = propvals.keys ()
    sortkeys.sort ()

    # Add all the properties to the table
    tab = ()
    for key in sortkeys:
        if key == "Comment":
            tab += ((key, "<textarea name=%sProp_%s cols=64 rows=4>%s</textarea>"
                % (prefix, propvals[key]["Id"], propvals[key]["Value"])),)
        else:
            tab += ((key, "<input type=text name=%sProp_%s value=\"%s\" size=64>"
                 % (prefix, propvals[key]["Id"], propvals[key]["Value"])),)

    # Create list of items that aren't currently used
    InstDB.Cur.execute ("SELECT PropId, PropName FROM Props ORDER BY PropName")
    options = []
    for row in InstDB.Cur.fetchall():
        if row["PropName"] not in sortkeys:
            options += (row["PropId"], row["PropName"])

    # If unused options, add a property add selection line
    if options:
        options = [0, "-- Not Selected --"] + options
        tab += (("Add", form_select ("%sProp_Add" % prefix, options, 0) \
                + "<input type=text name=%sProp_AddValue size=32>" % prefix),)
    return tab

def props_html_cells (proprows, prefix=""):
    "Create HTML table cells for item properties"

    # Add all the properties to a hash
    propvals = {}
    for prop in proprows:
        propvals[string.capitalize (prop["PropName"])] \
            = cgi.escape (prop["PropValue"])

    # Sort the property name keys
    sortkeys = propvals.keys ()
    sortkeys.sort ()

    # Add all the properties to the table
    tab = ()
    for key in sortkeys:
        tab += ((key, propvals[key]),)
    return tab

# Updates extra properties from a form submission
def props_update (itemId, fields):
    # Create dictionary of valid PropName:PropId
    InstDB.Cur.execute ("SELECT PropId FROM Props")
    validProps = []
    for row in InstDB.Cur.fetchall ():
        validProps += (row["PropId"],)

    # Look for all form fields starting with 'Prop_' prefix.
    prefix = "Prop_"
    for propId in validProps:
        val = fields.get ("Prop_" + str (propId), None)
        if val:    # Value set?  Try and update it first.
            c = InstDB.Cur.execute ("UPDATE ItemProps SET PropValue=%s"
                                    " WHERE ItemId=%s && PropId=%s",
                                    (val, itemId, propId))
            if c == 0:   # Did not update?  Possibly the same value.
                # Only insert if not already in table
                InstDB.Cur.execute ("SELECT COUNT(*) AS count FROM ItemProps"
                                    " WHERE ItemId=%s && PropId=%s",
                                    (itemId, propId))
                if InstDB.Cur.fetchone()["count"] == 0:
                    InstDB.Cur.execute ("INSERT INTO ItemProps"
                                        " VALUES(%s,%s,%s)",
                                        (itemId, propId, val))
        else:      # Value not set, remove it
            InstDB.Cur.execute ("DELETE FROM ItemProps WHERE ItemId=%s"
                                " && PropId=%s", (itemId, propId))

    # Check if a field has been added
    propAdd = fields.get ("Prop_Add", None)
    if propAdd: propAdd = int (propAdd)
    val = fields.get ("Prop_AddValue", None)
    if propAdd and val and propAdd in validProps:
        InstDB.Cur.execute ("INSERT INTO ItemProps VALUES(%s,%s,%s)",
                            (itemId, propAdd, val))

# Only need one instance of these helpers (FIXME - Better way to do this?)
PatchHelper = PatchHelperClass ()
InstHelper = InstHelperClass ()
SampleHelper = SampleHelperClass ()
PropHelper = PropHelperClass ()
RatingHelper = RatingHelperClass ()
UsersHelper = UsersHelperClass ()
