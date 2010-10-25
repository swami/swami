#!/usr/bin/python
#
# patchesdb_taskd.py - Patches database task daemon
# Waits for queued tasks and executes them (such as import tasks).
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

# Task polling interval in seconds
TASK_POLL_INTERVAL = 5

# Database connection retry interval in seconds
CONNECT_RETRY_INTERVAL = 60

import sys
sys.path += ("..",)         # Add parent directory to module search path

import InstDB
import PatchImport
import time
import MySQLdb
import traceback
import smtplib
import os

taskTypes = '"Import","Activate","Email"'

def log (msg):
    sys.stderr.write (msg + "\n")

def format_exc ():
    lstr = traceback.format_exception (sys.exc_type, sys.exc_value, sys.exc_traceback)
    return ''.join (lstr)               # Concatenate string list

def send_email (row):
    # Fetch users email address
    InstDB.Cur.execute ("SELECT UserEmail FROM Users WHERE UserId=%s", row["UserId"])
    UserEmail = InstDB.Cur.fetchone()["UserEmail"]

    datestr = time.strftime ("%a, %d %b %Y %H:%M:%S +0000", time.gmtime())

    # Add From, To and Date headers to message
    content = "From: %s\r\nTo: %s\r\nDate: %s\r\n" \
      % (InstDB.NoReplyEmail, UserEmail, datestr) + row["Content"]

    smtp = smtplib.SMTP (InstDB.SmtpHost)
    smtp.sendmail (InstDB.NoReplyEmail, UserEmail, content)
    smtp.quit ()

def import_queued_task (FileName, UserId, QueueId):
    global PatchTypes

    # Construct file path "/import-path/QueueId"
    queueIdPath = InstDB.ImportPath + os.sep + str (QueueId)
    importFileName =  queueIdPath + "-file"

    # Rename file from incoming to import-path/<QueueId>-file
    os.rename (InstDB.IncomingPath + os.sep + FileName, importFileName)

    # Create extract directory (queueId + '-extract')
    extractDir = queueIdPath + "-extract"
    os.mkdir (extractDir)

    # Handle archive/raw instrument file accordingly
    if not PatchImport.extract_archive (importFileName, FileName, extractDir):
        os.rmdir (extractDir)
        raise InstDB.ImportError, "Unknown file type"

    # Import the instrument information
    ItemId = PatchImport.import_patch (extractDir, UserId)

    # Move original file to original/<ItemId> (purged after some time)
    os.rename (importFileName, InstDB.OriginalPath + os.sep + str (ItemId))

    return ItemId


# Main outer loop (persistently tries to connect to database)
while True:
    try:
        InstDB.connect ()
    except MySQLdb.Error, detail:
        log ("Failed to connect to database: %s" % detail)
        time.sleep (CONNECT_RETRY_INTERVAL)
        continue

    # Inner loop polls for active tasks and executes them
    while True:
        try:
            InstDB.Cur.execute ("SELECT * FROM Queue"
                                " WHERE Status='Queued' && Type IN (%s)"
                                " ORDER BY QueueId" % taskTypes)
            QueueId = 0

            for row in InstDB.Cur.fetchall():       # For each queued task
                QueueId = row["QueueId"]

                # Set status to running (if not already by another process)
                c = InstDB.Cur.execute ("UPDATE Queue SET Status='Running'"
                                        " WHERE QueueId=%s && Status='Queued'",
                                        QueueId)
            
                # Skip if task was already taken by another process
                if c != 1: continue

                log ("Executing queued task %d of type '%s'"
                     % (QueueId, row["Type"]))

                try:
                    if row["Type"] == "Import":
                        import_queued_task (row["FileName"], row["UserId"],
                                            row["QueueId"])
                    elif row["Type"] == "Activate":
                        PatchImport.activate_patch (row["ItemId"])
                    elif row["Type"] == "Email": send_email (row)
                    else: raise ValueError, "Unsupported task type"

                    # Delete task
                    InstDB.Cur.execute ("DELETE FROM Queue WHERE QueueId=%s",
                                        row["QueueId"])
                    log ("Task succeeded")
                except:
                    lstr = format_exc ()
            
                    msg = "Internal Error"
            
                    # Set the task to Error status, log message and traceback
                    InstDB.Cur.execute ("UPDATE Queue SET Status='Error',ErrorMsg=%s,"
                                        "Log=%s WHERE QueueId=%s", (msg, lstr, QueueId))
            
                    log (msg + ":\n" + lstr)


        except MySQLdb.Error, detail:
            log ("Database error while processing queued import task: %s"
                 % detail)
            break             # Break out of inner loop (connect again)

        time.sleep (TASK_POLL_INTERVAL)
