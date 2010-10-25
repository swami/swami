#!/usr/bin/python
#
# PatchImport.py - Instrument import routines.
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
#
# Import stages:
# --- Extract - extract_archive (importFile, origFile, extractDir) ---
# Original archive is extracted to a temporary folder (copied if raw instrument)
#
# --- Import - import_patch (filepath, userId) ---
# 'filepath' contains one or more files, one of which is an instrument file.
# The largest instrument file is imported into the database.
# Extract directory is moved to activate/<PatchId>-files
# State of PatchInfo set to 'Imported'
#
# --- Activate - activate_patch (patchId, cramFile) ---
# Files in activate/<PatchId>-files are ZIPed and CRAMmed (not yet complete).
# RAW file info added to Files table for patchId.
# CRAM file is moved to patches/<cramFile> (not yet complete)
# ZIP file is moved to patches/<zipFile>
# PatchInfo State='Active', ZipSize=Zip file size,
#   CramSize=CRAM file size (not yet), FileSize=raw size sum
#
# Web import procedure:
# - User uploads files via a transport like anonymous FTP
# --- import.py ---
# - User selects files for import from import.py page
# - Selected files are added to job "Queue" table (Status=Queued) for import
#   (using "FileName" field)
# --- patchesdb_taskd.py ---
# - Queued import task is exected for selected file: Status=Running
# - Import file renamed to "import/<QueueId>-file" (example: "import/250-file")
# - "import/<QueueId>-extract" directory is created
# - Import file is extracted to extract directory if necessary (raw patch files
#   are just copied)
# - Largest instrument file is imported into database using libInstPatch
# - Extract directory moved to activate/ directory as "<PatchId>-files"
# - Patch is marked as 'Imported' in PatchInfo table
# - Original uploaded file moved to "original/<PatchId>"
# --- import.py ---
# - User completes editing of imported instrument files and submits,
#   an "Activate" task is added to the job Queue
# --- patchesdb_taskd.py ---
# - patchesdb_taskd.py executes queued activate task: files re-compressed with
#   ZIP and CRAM (not yet) and moved to download directory, extracted files
#   cleaned up, patch state changed to 'Active'

import getopt, os, os.path, re
import subprocess
import MySQLdb
import ipatch
import gobject
import tarfile
import shutil
import md5

import InstDB

# Allowed patch types
PatchTypes = (ipatch.SF2File.__gtype__, ipatch.DLSFile.__gtype__,
              ipatch.GigFile.__gtype__)

# DLS info properties
dls_info = ("source", "medium", "date", "product", "source-form",
            "engineer", "technician", "subject", "archive-location",
            "name", "copyright", "genre", "commissioned",
            "software", "comment", "artist", "keywords")

# Useful properties for each patch object type
obj_propnames = {
    ipatch.SF2 : ("date", "product", "author", "name", "copyright",
                  "software", "comment", "engine"),
    ipatch.DLS2 : dls_info,
    ipatch.SF2Preset : ("name", "bank", "program"),
    ipatch.SF2Sample : ("name", "root-note"),
    ipatch.DLS2Inst : ("name", "bank", "program"),
    ipatch.DLS2Sample : ("name", "root-note"),
    ipatch.GigInst : ("name", "bank", "program"),
    ipatch.GigSample : ("name", "root-note")
}


def get_props (obj, inlist):
    propvals = {}
    for pspec in gobject.list_properties (obj):
        if pspec.name in inlist:
            val = obj.get_property (pspec.name)
            if val and type (val) == str: val = val.strip ()
            if val:
                s = pspec.name.replace ('-', ' ').title ().replace (' ', '')
                propvals[s] = val
    return propvals


def save_props (id, props):
    for prop in props.items ():
        InstDB.Cur.execute ("SELECT PropId FROM Props WHERE PropName=%s",
                            (prop[0]))
        row = InstDB.Cur.fetchone ()

        if not row:
            InstDB.Cur.execute ("INSERT INTO Props (PropName) VALUES (%s)",
                                (prop[0]))
            propid = InstDB.DB.insert_id ()
        else: propid = row["PropId"]

        InstDB.Cur.execute ("INSERT INTO ItemProps (ItemId, PropId, PropValue)"
                            " VALUES (%s, %s, %s)", (id, propid, prop[1]))

def strip_file_ext (filename, ext):
    filename = os.path.basename (filename)
    match = re.search ("(?i).*(\.%s)$" % ext, filename)
    if match and match.start (1) != 0:
        filename = filename[:match.start (1)]

    if len (filename) == 0 or filename == "." or filename == "..":
        filename = "untitled"

    return filename

def find_patch (filepath):
    """
    Find a patch file in a directory or verify that a file is a patch
    filepath -- Path to a patch file to verify or directory to find patch in
    Returns: Full path to found patch file or None if not found
    """

    # Construct list of files
    if os.path.isfile (filepath):       # filepath is a single file?
        files = (filepath,)
    else: files = map (lambda s: filepath + os.sep + s, os.listdir (filepath))

    foundname = None
    foundsize = 0

    for fullpath in files:
        file = ipatch.File ()
        file.open (fullpath, "r")

        # See if libInstPatch can identify the file type
        fileType = file.identify ()

        if fileType in PatchTypes:
            size = os.path.getsize (fullpath)

            # Skip if already found and larger
            if foundname and size < foundsize: continue

            foundname = fullpath
            foundsize = size

    return foundname

def extract_archive (importFile, origFile, extractDir):
    """
    Extracts an archive to a directory or just copies file if its a raw
    instrument file.

    importFile: Source file to extract (if raw instrument, its just copied)
    origFile: Original file name (used if archive does not contain file names,
              gzip, bzip and sfArk), should not contain path elements
    extractDir: Destination extraction directory
    Returns: True if suppported archive file type, False if not
    """

    # Check if libInstPatch can identify the file
    file = ipatch.File ()
    file.open (importFile, "r")
    fileType = file.identify ()

    # Is it an uncompressed patch file?
    if fileType in PatchTypes:
        # copy raw file into extract directory (using its original name)
        shutil.copy (importFile, extractDir + os.sep + origFile)

    # Is it a CRAM file?
    elif fileType == ipatch.CramFile.__gtype__:
        file = file.convert_type (fileType) # Convert to IpatchFile sub type
        conv = ipatch.CramDecoderConverter () # CRAM decoder converter
        conv.set_property ("path", extractDir) # Set extract directory
        conv.set_property ("strip-paths", True) # Strip paths
        conv.add_input (file)       # Add CRAM file as input
        conv.convert ()             # Decode all files to extractDir

    elif tarfile.is_tarfile (importFile): # Is it tar, tar/gzip or tar/bzip2?
        tar = tarfile.open (importFile, "r")

        for name in tar.getnames (): # Extract each file to extractDir
            tarfile.extract (name, extractDir)

        tar.close ()

    else:                   # Not a tar file, what is it? - Use 'file' utility
        fd = subprocess.Popen([InstDB.FILE_CMD, '-b', importFile], stdout=subprocess.PIPE).stdout
        out = fd.read ()

        if out[:5] == "gzip ":
            fd = gzip.open (importFile)

            fname = extractDir + os.sep + strip_file_ext (origFile, "gz")
            outfd = file (fname)

            while 1:
                data = fd.read (COPYBUFSIZE)
                if not data: break
                outfd.write (data)

            outfd.close ()
            fd.close ()
        elif out[:6] == "bzip2 ":
            fd = bz2.BZ2File (importFile)

            fname = extractDir + os.sep + strip_file_ext (origFile, "bz2")
            outfd = file (fname)

            while 1:
                data = fd.read (COPYBUFSIZE)
                if not data: break
                outfd.write (data)

            outfd.close ()
            fd.close ()
        elif out[:4] == "Zip ":
            retval = subprocess.call ([InstDB.UNZIP_CMD, "-d", extractDir, "-j",
                                      importFile])
            if retval > 2:
                raise InstDB.ImportError, "Unzip of archive file failed (%d)" % retval
        elif out[:4] == "RAR ":
            retval = subprocess.call ([InstDB.UNRAR_CMD, "e", importFile,
                                      extractDir])
            if retval != 0:
                raise InstDB.ImportError, "Unrar of archive file failed (%d)" % retval
        elif out[:6] == "sfArk ":
            # sfarkxtc utility always treats output file name as relative path
            fname = os.path.basename (extractDir) + os.sep \
                + strip_file_ext (origFile, "sfark") + ".sf2"
            retval = subprocess.call ([InstDB.SFARKXTC_CMD, importFile, fname])
            if retval != 0:
                raise InstDB.ImportError, "sfArkXTc failed to extract file (%d)" % retval
        else:
            return False                # Unknown archive file

    return True


def import_patch (filepath, userid):
    """
    Load a file into a patch object
    filepath -- Path to directory (if multiple files, larger
                patch files take precedence for information import)
    userid -- ID of the user importing the file
    Returns -- patch ID
    """

    # Find the patch file
    patchFileName = find_patch (filepath)

    if not patchFileName:
        raise InstDB.ImportError, "Valid patch file not found in files"

    file = ipatch.ipatch_file_identify_open (patchFileName, 'r')

    # Convert file to IpatchBase derived object
    patch = ipatch.ipatch_convert_object_to_type (file, ipatch.Base)

    if type (patch) == ipatch.SF2:
        patch_type = "sf2"
        inst_type = ipatch.SF2Preset
        sample_type = ipatch.SF2Sample
        proplist = obj_propnames[ipatch.SF2]
        extmatch = "sf2"
    elif type (patch) == ipatch.DLS2:
        patch_type = "dls"
        extmatch = "dls[12]?"
        inst_type = ipatch.DLS2Inst
        sample_type = ipatch.DLS2Sample
        proplist = obj_propnames[ipatch.DLS2]
    elif type (patch) == ipatch.Gig:
        patch_type = "GIG"
        extmatch = "gig"
        inst_type = ipatch.GigInst
        sample_type = ipatch.GigSample
        proplist = obj_propnames[ipatch.DLS2]
    else:
        raise InstDB.ImportError, "Unhandled file type '%s'" \
              % gobject.type_name (type (patch))

    # Get properties for the patch object
    props = get_props (patch, proplist)

    # Insert the patch item into the Items table
    InstDB.Cur.execute ("INSERT INTO Items (ItemType) VALUES ('Patch')")
    patchid = InstDB.DB.insert_id ()

    # Get the name and remove from props list
    if props.has_key ("Name"):
        name = props["Name"]
        del props["Name"]
    else: name = "<Untitled>"

    # Look for author string
    author = ""
    for authorProp in ("Artist", "Author"):
        if props.has_key (authorProp):
            if props[authorProp].strip ():
                author = props[authorProp].strip ()
            del props[authorProp]

    # Strip file extension
    fname = os.path.basename (patchFileName)
    match = re.search ("(?i).*(\.%s)$" % extmatch, fname)
    if match and match.start (1) != 0:
        fname = fname[:match.start (1)]

    # Insert patch info into PatchInfo database
    InstDB.Cur.execute ("INSERT INTO PatchInfo"
                        " (PatchId,UserId,PatchAuthor,FileName,PatchType,"
                        "PatchName,UserImported,DateImported)"
                        " VALUES (%s, %s, %s, %s, %s, %s, %s, NULL)",
                        (patchid, userid, author, fname, patch_type,
                         name, userid))

    # Save all properties
    save_props (patchid, props)

    # Save info for all instruments
    for inst in patch.get_children (inst_type):
        props = get_props (inst, obj_propnames[inst_type])

        InstDB.Cur.execute ("INSERT INTO Items (ItemType) VALUES ('Inst')")
        instid = InstDB.DB.insert_id ()

        # Get the name and remove from props list
        if props.has_key ("Name"):
            name = props["Name"]
            del props["Name"]
        else: name = ""

        InstDB.Cur.execute ("INSERT INTO InstInfo"
                            " (InstId, PatchId, Bank, Program, InstName)"
                            " VALUES (%s, %s, %s, %s, %s)",
                            (instid, patchid, props.get ("Bank", 0),
                             props.get ("Program", 0), name))

        # Remove the already used properties
        for key in ("Bank", "Program"):
            if props.has_key (key): del props[key]

        # Save rest of the instrument properties
        save_props (instid, props)


    # Save info for all samples
    for sample in patch.get_children (sample_type):
        rate = sample.get_property ("sample-rate")
        samdata = sample.get_property ("sample-data") # Get sample data object

        # Get first sample store
        store = samdata.get_children (ipatch.SampleStore)
        if store: store = store[0]
        if not store: continue

        channels = store.get_property ("channels")

        width = store.get_property ("width")
        if width == ipatch.SAMPLE_8BIT: width = "8bit"
        elif width == ipatch.SAMPLE_16BIT: width = "16bit"
        elif width in (ipatch.SAMPLE_24BIT, ipatch.SAMPLE_REAL24BIT):
            width = "24bit"
        elif width == ipatch.SAMPLE_32BIT: width = "32bit"
        elif width == ipatch.SAMPLE_FLOAT: width = "float"
        elif width == ipatch.SAMPLE_DOUBLE: width = "double"
        else:
            raise RuntimeWarning, "Unhandled sample bit depth enum %d" % width
            continue

        # Sample size in bytes is frame size * number of frames
        size = store.get_frame_size () * samdata.get_property ("sample-size")

        props = get_props (sample, obj_propnames[sample_type])

        # Get the name and remove from props list
        if props.has_key ("Name"):
            name = props["Name"]
            del props["Name"]
        else: name = "<Untitled>"

        InstDB.Cur.execute ("INSERT INTO Items (ItemType) VALUES ('Sample')")
        sampleid = InstDB.DB.insert_id ()

        InstDB.Cur.execute ("INSERT INTO SampleInfo (SampleId,PatchId,Format,"
                            " SampleSize,Channels,RootNote,SampleRate,SampleName)"
                            " VALUES (%s, %s, %s, %s, %s, %s, %s, %s)",
                            (sampleid, patchid, width, size, channels,
                             props.get ("RootNote", 60), rate, name))

        # Remove the already used properties
        if props.has_key ("RootNote"): del props["RootNote"]

        # Save rest of the sample properties
        save_props (sampleid, props)

    # Move extract directory to activate/<PatchId>-files
    os.rename (filepath, InstDB.ActivatePath + os.sep + str (patchid) + "-files")

    # Update PatchInfo State to "Imported"
    InstDB.Cur.execute ("UPDATE PatchInfo SET State='Imported'"
                        " WHERE PatchId=%s", patchid)

    return patchid


def activate_patch (patchId):
    """
    Activate an instrument by ZIPing and CRAMing the contents of the directory
    ActivatePath/<patchId>-files, updating PatchInfo record and adding file list
    to Files table.

    patchId: ID of existing PatchInfo record in database to activate
    """

    # Construct ZIP and CRAM file names from FileName field in PatchInfo table
    InstDB.Cur.execute ("SELECT FileName, PatchType FROM PatchInfo"
                        " WHERE PatchId=%s", patchId)
    row = InstDB.Cur.fetchone ()
    zipfile = row["FileName"] + "." + row["PatchType"] + ".zip"
    cramfile = row["FileName"] + "." + row["PatchType"] + ".cram"

    patchIdPath = InstDB.ActivatePath + os.sep + str (patchId)
    patchFilePath = patchIdPath + "-files"

# Disabled for now until libInstPatch CRAM support is ready
    # cramSize = cram_files (patchIdPath, patchFilePath, cramfile)
    # cramMd5 = md5file (InstDB.PatchPath + os.sep + cramfile)
    cramSize = 0
    cramMd5 = ""

    # Create zip archive
    zipSize = zip_files (patchIdPath, patchFilePath, zipfile)
    zipMd5 = md5file (InstDB.PatchPath + os.sep + zipfile)

    # Add files to Files table
    rawsum = 0
    for fname in os.listdir (patchFilePath):
        fullpath = patchFilePath + os.sep + fname
        size = os.path.getsize (fullpath)
        rawsum += size

        md5str = md5file (fullpath)

        InstDB.Cur.execute ("INSERT INTO Files (PatchId, FileName, FileSize, Md5)"
                            " VALUES (%s, %s, %s, %s)",
                            (patchId, fname, size, md5str))

    # Update PatchInfo State to "Active" and set CramSize
    InstDB.Cur.execute ("UPDATE PatchInfo SET State='Active', CramSize=%s, CramMd5=%s,"
                        " ZipSize=%s, ZipMd5=%s, FileSize=%s WHERE PatchId=%s",
                        (cramSize, cramMd5, zipSize, zipMd5, rawsum, patchId))

    # Delete the extracted files directory
    shutil.rmtree (InstDB.ActivatePath + os.sep + str (patchId) + "-files")


def cram_files (patchIdPath, patchFilePath, cramfile):
    """CRAM files in patchFilePath to archive '<patchIdPath>.cram'"""

    # Convert files to objects and add to archiveFiles array
    archiveFiles = ()
    for fname in os.listdir (patchFilePath):
        file = ipatch.File ()
        file.open (patchFilePath + os.sep + fname, "r")

        # See if libInstPatch can identify the file type
        fileType = file.identify ()

        # Convert instrument files to their native IpatchFile type
        if fileType in PatchTypes:
            file = file.convert_type (fileType)

        archiveFiles += (file,)        # Add file object to archive file list

    # CRAM the files
    cram = ipatch.CramFile ()
    tempCramPath = patchIdPath + ".cram"
    cram.open (tempCramPath, "w")

    conv = ipatch.CramEncoderConverter () # CRAM encoder converter
    conv.set_property ("comment", InstDB.CramFileComment)
    conv.add_output (cram)

    # Add input file objects to converter
    for file in archiveFiles:
        conv.add_input (file)

    conv.convert ()                 # Do the conversion (CRAM encoding)

    # Get size of CRAM file
    cramSize = os.path.getsize (tempCramPath)

    # Move the cram file to patches/ (make sure file doesn't already exist)
    cramFilePath = InstDB.PatchPath + os.sep + cramfile
    if os.path.exists (cramFilePath):
        raise InstDB.ImportError, "File '%s' already exists in patch directory"\
            % cramfile
    os.rename (tempCramPath, cramFilePath)

    return cramSize


def zip_files (patchIdPath, patchFilePath, zipfile):
    """ZIP files in patchFilePath to archive '<patchIdPath>.zip'"""

    tempZipPath = patchIdPath + ".zip"

    # ZIP the files (with ZIP file comment on stdin)
    process = subprocess.Popen ([InstDB.ZIP_CMD, "-z", tempZipPath]
                                + os.listdir (patchFilePath),
                                cwd=patchFilePath, stdin=subprocess.PIPE)

    process.stdin.write (InstDB.ZipFileComment)
    process.stdin.close ()
    process.wait ()

    if process.returncode > 2:
        raise InstDB.ImportError, "Zip of archive failed (%d)" % process.returncode

    # Get size of ZIP file
    zipSize = os.path.getsize (tempZipPath)

    # Move the ZIP file to patches/ (make sure file doesn't already exist)
    zipFilePath = InstDB.PatchPath + os.sep + zipfile
    if os.path.exists (zipFilePath):
        raise InstDB.ImportError, "File '%s' already exists in patch directory"\
            % zipfile
    os.rename (tempZipPath, zipFilePath)

    return zipSize


def md5file (filename):
    fh = open (filename, 'r')

    m = md5.new ()

    while True:
        data = fh.read (65536)
        if not data: break;

        m.update (data)

    return m.hexdigest ()
