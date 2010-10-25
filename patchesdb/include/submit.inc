<h3>Introduction</h3>

Thank you for your interest in contributing content to the Resonance Instrument
Database. Feel free to submit instrument content created by yourself or others
but make sure that it is OK to distribute publicly. If you need any assistance,
have a feature request or compliment feel free to make use of the
<a href="forums/">forums</a> or contact the
<a href="mailto:sounds-admin@resonance.org">admin</a>.

<p>

<h3>File formats</h3>

The Resonance Instrument Database currently accepts the following instrument
formats:<br>
<b>DLS</b> (Downloadable Sounds v1 or v2), <b>SF2</b> (SoundFont v2.x only)
and <b>GIG</b> (GigaSampler, some versions probably don't work).

<p>

Instrument files can be uploaded raw or optionally be compressed with
any of the following formats:<br>
tar, gzip, bzip2, tar/gzip, tar/bzip2, RAR, ZIP or sfArk.

<p>

<b>Note:</b> The import process will attempt to automatically uncompress
archives and do the "right thing". This could easily fail or not provide the
flexibility needed to cleanly import the instrument. If you find something is
not working right, contact the admin for manual intervention.

<p>

All uploads will be decompressed and then re-compressed with
ZIP (and in the future: CRAM).  It is recommended that archives contain only
one instrument file (if it contains more then one, then only the largest will
be imported into the database), but may optionally contain additional arbitrary files
(a README text file for example) which will be distributed in the
resulting archive as well.  It is recommended that you prefix
additional files with the name of your instrument as a convenience to
the user extracting into a populated directory (for example, if an instrument
is called "Fluid.sf2" a "Fluid-README.txt" would be appropriate).
Paths inside archives will not be preserved (files will extract to
current directory).

<h3>Uploading Files</h3>

Files should be uploaded to
<a href="ftp://sounds.resonance.org/incoming">ftp://sounds.resonance.org/incoming</a>.

After completing your upload(s), login and go to the content
<a href="patches.py?Action=submit">Submit</a> page. Check the box
next to each file that you uploaded. <b>Please select only your
files.</b> Importing other user's files will likely be rather annoying
to them. If you find you have made a mistake, contact the admin (don't worry
I'm fairly forgiving :)

<p>

Once you have selected your files, you can click the "Import and/or Refresh"
button.  Note that any unclaimed uploads may be deleted after 24 hours.

<h3>Queued Tasks</h3>
Imported files are queued for import in first come first serve
order.  The queue positions and status will be displayed for each file
you are importing.  File "Activation" tasks will also appear in this
list.  The queue positions are shown which include other user's tasks as well.
If you receive an "Error" status, please contact the admin so that the problem
can be corrected.

<h3>Activating Your Files</h3>
After a file has been extracted and imported it will appear under the
"Files pending activation" heading.  Here you can edit the names
of the files that will be compressed in the archive and the instrument
file information such as category, license, etc.
This information can be changed at a later time (except for the file names).

<p>

All of your current files pending activation will be shown.
Once you have finished editing the information for a file click
its "Activate" check box.  An "activation" task will then be added
to the queue, which when serviced will compress the files into an
archive and then activate the file, making it available to other
users.
