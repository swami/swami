# Swami - README
### Copyright (C) 1999-2021 Element Green and others

http://www.swamiproject.org

[![Build Status](https://dev.azure.com/tommbrt/tommbrt/_apis/build/status/swami.swami?branchName=master)](https://dev.azure.com/tommbrt/tommbrt/_build/latest?definitionId=2&branchName=master)

_Note that this readme might be outdated_


## 1. What is Swami?

Swami (Sampled Waveforms And Musical Instruments) is a SoundFont
editor. SoundFont files are a collection of audio samples and
other data that describe instruments for the purpose of composing
music. SoundFont files do not describe the music itself, but rather the
sounds of the instruments. These instruments can be composed of any
digitally recordable or generated sound. This format provides a
portable and flexible sound synthesis environment that can be
supported in hardware or software.

## 2. What happend to the Smurf SoundFont Editor?

Nothing actually. Since I was already doing an entire re-code of Smurf
I decided it would be good to change the name to give the program a
fresh start and because I'm planning to make it more than just a
SoundFont editor. Also the word 'Smurf' is copyrighted so I thought a
change wise in this day and age of law suits. Swami does not yet
completely replace Smurf in functionality, but it will.

## 3. License

See [Copying](https://github.com/swami/swami/blob/master/COPYING).

## 4. Changes from Smurf

Most of the changes have occured in the programming architecture. Much
of this work has been done in preparation for really cool features
(TM) :) The SoundFont editing code is now completely abstracted from
the GUI in separate shared libraries making things like: scripting
support, multiple client concurrent access to SoundFont objects and SwamiJam
(networked semi-realtime composition with friends) more feasible. Most
importantly things are much cleaner now so I don't go insane in attempts
to add functionality.

New features:
- FluidSynth soft synth support (actually its required) which allows any
  OS supported sound card to be used and gives us modulators
  (real time effects) and routeable digital audio output for adding additional
  effects and for easy recording.
- More operations work for multiple items. Including loading/saving/closing
  of multiple SoundFont files, loading multiple samples, and
  setting/viewing properties on multiple items.
- Modulator editor for editing real time control parameters
- Default toggle buttons for generator (effect) controls

Things missing (as of this README):
- Undo support
- virtual SoundFont bank support
- OSS AWE/SB Live! wavetable backend


## 5. Requirements

Look at the INSTALL file for instructions on compiling and installing Swami
and for more details on software requirements.

Version numbers listed are what I developed Swami on, older or newer versions
of the listed software may or may not work, but give it a try! And do let me
know if you encounter any problems.

Swami has the following requirements:
- Linux, Windows and Mac OS X.  Probably anywhere else GTK+ will run.
- GTK+ v2.0
- FluidSynth v2.0 (software wavetable synthesizer)
- libsndfile

GTK homepage: http://www.gtk.org
FluidSynth homepage: http://www.fluidsynth.org
libsndfile homepage: http://www.mega-nerd.com/libsndfile
ALSA homepage: http://www.alsa-project.org
OSS homepage: http://www.4front-tech.com


## 6. Supported sound cards

Any FluidSynth supported sound card (i.e. works under ALSA or OSS in Linux), since
Swami uses FluidSynth which does all synthesis in software.  A wavetable
backend is planned for using hardware based SoundFont synthesizers like the
SB AWE/Live! for those who don't want to use their precious CPU cycles :)


## 7. Features

Current features include:
- Loading of multiple SoundFont 2.x files *.SF2
- Saving of multiple SoundFont files
- Copy Samples/Instruments/Presets between and within SoundFont objects
- Load and export multiple audio sample files (WAV, AU, etc)
- View/set multiple Preset, Instrument and Sample parameters
- A GTK piano widget to play notes with computer keyboard/mouse
- Wavetable loading of entire SoundFont
- Piano MIDI controls (Pitch bender amount, volume etc.)

Features provided by FluidSynth:
- Flexible driver system that supports OSS, ALSA, LADSPA, Jack, Midishare, etc
- Real time effects via modulators and C API

## 8. Future plans

The following are in the works or planned:
- Undo support (just haven't written a front end for it really)
- Copy/Paste generator parameters
- Graphical manipulation of generator parameters

Was part of Smurf, not yet added:
- Loading, saving and editing of virtual bank (*.bnk) files
- Multiple undo/redo tree system, allowing for multiple redo "branches"
- Sample cut support
- Sample waveform generation
- International language support (currently disabled until there are some
  updated translations, contact me if interested)


## 9. Troubleshooting drivers (or why does Swami crash on startup?)

[Some of these switches haven't been enabled yet!]

Some new command line switches were added to help with trouble shooting
problems with Swami. Crashes experienced on startup of Swami are often caused
by audio drivers.  Type "swami --help" to get a list of usable switches.
Since Swami, by default, tries to open the drivers on start up, a way to
disable this behavior was added. "swami -d" will cause Swami to start, but
drivers will not automatically be opened. Once started you can change your
driver preferences (set certain ones to NONE) to try to determine which
driver is crashing. Then issue a Control->"Restart Drivers" to cause the
drivers to be re-opened.

Swami v0.9.0pre1
Copyright 1999-2002 Josh Green <jgreen@users.sourceforge.net>
Distributed under the GNU General Public License
Usage: swami [options]
Options:
-h, --help       Display this information
-d               Disable drivers (try if Swami crashes on start up)
-c               Ignore preferences and use factory defaults


## 10. Using Virtual Keyboard interface

[Most of this is true, although some things haven't been implemented yet]

The wavetable and piano interface have been changed in Smurf v0.52.  Some of
the new features include:
* MIDI channel control (on toolbar, titled "Chan")
	Sets what MIDI channel the piano plays on. All other MIDI controls
	(bank, preset and MIDI controls on pop up menu) are remembered for
	each channel.
* MIDI bank and preset controls (also on toolbar)
	Sets the current bank and preset for the selected MIDI channel. These
	are often changed automatically by the user interface when selecting
	items in the SoundFont tree, they can be changed manually as well.
* Entire SoundFont loading
	Allows you to load an entire SoundFont. This is particularly useful
        to make a SoundFont available for sequencing with other programs.
* Controls for changing how interface works with wavetable
	* Piano follows selection
	
		When enabled: virtual keyboard will "follow" the SoundFont
		tree selection. This will cause the selected item to be the one
		heard on the piano. If selected item is a Preset in a loaded
		SoundFont, then the piano will select the Bank:Preset of that
		item. Otherwise it is set to the Temporary Audible Bank:Preset
		(configurable from preferences, defaults to 127:127). By
		disabling this feature you can have more control over what is
		being played by the virtual keyboard.

	* Auto load temp audible
	
		When enabled: selected items in the SoundFont tree that aren't
		Presets in a loaded SoundFont, are automatically loaded into
		the wavetable device. They are mapped to a temporary
		Bank:Preset which is configurable from Preferences. Disabling
		this allows for faster editing, if one does not care to hear
		items.

* Temporary audible -
The temporary audible enables you to listen to components of a SoundFont that
aren't directly mapped to a MIDI Bank:Preset. This is done by designating a
Bank:Preset for temporary use. It defaults to 127:127 but can be changed
in Preferences. When a non-Preset or a Preset that isn't in a loaded SoundFont
is loaded (either from the "Auto load temp audible" option or manually from the
"Wavetable Load" item on the right click menus), it is mapped to this temporary
bank and preset number.


## 11. Creating Preset/Instrument zones

To add zones to an Instrument hold down the CTRL key and select the samples
you want to be added (SHIFT works too for selecting a range of items). Then
"Right Click" the Instrument you want to add the samples to.
Select "Paste" from the pop up menu. A zone will be created for each
Sample (or Instrument zone) you selected. This same procedure can be applied to
Presets using Instruments as zones.
To create a "Global Zone", "Right Click" on the Preset or Instrument you want
to create the zone under. Select "Global Zone" from the pop up menu. Global
Zones are used to set "Default" generator (and modulator) parameters that the
other zones don't already have set.


## 12. Using the sample viewer

The sample viewer is used to change loop points for samples and
instrument zones.

Some things have changed in the sample viewer and probably will be
changing again. The sample zoom/unzoom is now performed with the SHIFT
key held down.  Regular left mouse clicks are now used for sample
selections for audio editing functions (currently only "cut").

Tips to using the sample viewer:

Zooming IN/OUT
Hold down SHIFT on the computer keyboard and click and hold the LEFT mouse
button in the sample window, a white line will appear to mark your click, move
the mouse to the left or the right to zoom into the sample. The distance you
move the mouse from the original point of your click determines the speed in
which the view zooms. If you then move the mouse to the other side of the line
the view will unzoom. The RIGHT mouse button does the same thing, except the
first direction you move the mouse unzooms the view.

Selecting audio data
Left click and drag to mark an area. An existing selection can be
changed by holding down CTRL and Left or Right clicking to change the
left or right ends of the selection respectively.


## 13. Undo system

**Not enabled yet!**

Swami has a rather flexible undo system. Many actions aren't currently
undo-able, but will be implimented soon. Currently all actions
relating to new SoundFont items, pasted SoundFont items, and deleted
items are undo-able.  The undo system is based on a tree rather than
queue or list. This means that multiple changes can be easily tested.

A traditional undo system is oriented as a list. Here is an example to help
understand the Swami undo tree system compared with the traditional one.

1. You change some parameters related to the Modulation Envelope of an
   instrument
2. You don't quite like the changes, so you undo them
3. You try some other values for the same Modulation Envelope

Lets say now you want to try the parameters in Step 1 again to compare to the
settings in Step 3. In a traditional undo system you would not be able to do
this because Step 3 would cause the "redo" information from Step 2 to be lost.
Using a tree, multiple branches of "state" information can be stored. So rather
than Step 3 "over writing" the redo info from Step 2, it would simply create
another "branch" in the tree.

Note: This was just an example of usage, but Swami doesn't support undo-ing of
parameter changes yet.

When just using the Undo/Redo menu items the undo system acts as a
traditional list oriented undo system.
The "Undo History" item on the Edit menu will bring up a dialog that
lists previous actions. The dialog buttons available are Undo, Redo,
Jump, Back, Forward, < (left arrow), > (right arrow).
The current location in the state history is indicated by 2 greater
than symbols ">>" in the left most column.

Undo and Redo act just like the menu items, undo-ing (moving up the
undo history) and redo-ing (moving down the undo history) the
next/previous action from the current location.
The "Jump" button will jump to the desired position in the undo
history, causing actions to be undone/redone as necessary to realize
the desired state.
The Back and Forward buttons act like the traditional web browser
function.  Currently the last 10 positions in the undo history are
remembered. This makes testing different settings easier. This isn't
really useful yet as the currently undo-able actions aren't really
ones you would wan't to test different settings with. But in the
future when generator parameters are tracked in the undo history, this
will be useful.
The < (left arrow) and > (right arrow) are the second and third column
headers of the history list. These columns are used to indicate other
branches in the tree. Items that have other "sibling" branches will
have a number in brackets (example: [ 1]) listed in the < (left arrow)
and/or > (right arrow) column(s). The left arrow switches to older
branches the right arrow to newer ones. Numbers in these columns
indicate how many alternate older or newer branches (left arrow or
right arrow respectively) are available.

To switch to an alternate branch, select an item that has a number in
one of the arrow columns. Click the corresponding arrow button at the
top of the list.  The selected item and all items below will be
changed to reflect the new branch.


## 14. Virtual Banks

**Not implemented yet in Swami!**

Virtual banks allow you to make a SoundFont bank from many other SoundFont
files. Swami currently supports loading and saving of the AWE .bnk format.
This is a simple text file format listing each Preset and where it can be found
SoundFont file:Bank:Preset and its destination Bank:Preset.

**NOTE:** Make sure you set the "Virtual SoundFont bank search paths" field
in Preferences. It should be a colon seperated list of where to find your SoundFont
files.  Example: "/home/josh/sbks/*:/tmp/sbks". Adding a '/*' to the end of a
path will cause that directory to be recursively scanned (all sub directories
under it).

**Current limitations (will be fixed in future)**

SoundFont files will not automatically be loaded with a virtual bank, so you'll
have to load them by hand.
Default SoundFont bank can't be cleared
Can't edit virtual bank mappings


#### Adding a Preset to a virtual bank
Simply select and paste Preset items into the Mapping section of the virtual
bank.

#### Setting default SoundFont
The default SoundFont sets the default Preset instruments. The Preset mappings
override the default Presets. To change the default SoundFont simply paste a
SoundFont item to the <DEFAULT> branch of the virtual bank.


## 15. SoundFont information, links and other resources

I wrote an "Intro to SoundFonts" and its available on the Swami homepage
http://swami.sourceforge.net. Its a general overview of what a SoundFont is.

More information on the SoundFont standard can be obtained from
www.soundfont.com. There is a PDF of the SoundFont technical specification
at www.soundfont.com/documents/sfspec24.pdf.  If you find that it has moved
then look under the Resources section off of the soundfont.com home page
(although it looks like you have to get a login to go there).

Thanks to Frank Johnson at EMU for clarifying this.


## 16. Thanks

Thanks to the following projects for making Swami possible:
- FluidSynth (its what makes the instruments sound :)
- gstreamer for gobject2gtk code and helpful examples of using GObject
- Glade GTK interface builder
- Gimp for graphics editing
- Blender for render of campfire in desert Splash image
- xmms code which I borrowed the multi selection file dialog modifications from
- Piano to computer keyboard mapping inspired from soundtracker


## 17. Trademark Acknowledgement

SoundFont is a registered trademark of E-mu Systems, Inc.
Sound Blaster Live! and Sound Blaster AWE 32/64 are registered trademarks of
Creative Labs Inc.
All other trademarks are property of their respective holders.


## 18. Contact

Contact me (Element Green) particularly if you are interested in helping
develop Swami. Programming, web page design, and
documentation are all things that are lacking, so if you have an
interest, do contact me. Also contact me if you find any bugs, have
trouble compiling on a platform you think should be supported, want to
tell me how much you like Swami, or have suggestions or ideas.

Email: element@elementsofsound.org

Swami homepage: http://www.swamiproject.org
