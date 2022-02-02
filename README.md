# xmplay-sidex-c64-sid-music-player-plugin
xmplay-sidex c64 sid music player plugin

https://iili.io/li1L5x.gif


SIDex Input Plug-in
=============================

This is brand spanking new plug-in to play SID files using the libsidplayfp-2.3.0 library.

- Fade-in support to try and remove clicks
- SLDB, STIL, BUGlist from HVSC supported
- Configurable SIDId support via sidid.cfg files
- stiltxt2md5 tool to support STIL outside of HVSC folders



STILtxt2md5
=============================

This is a simple commandline to convert the existing STIL.txt and BUGlist.txt to md5 versions 
that don't require the SID you are playing to be in the HVSC folder structure.

It adds a few hundred KB to the size but nothing too bad, it has been added as a command line
so you can run it again whenever you update your HVSC to the latest version.



SIDId Support
=============================

xmp-sidex does not keep a copy of the SIDId database internally but a sidid.cfg file is
included in the archive. Copy the sidid.cfg folder to wherever you point the HVSC
DOCUMENTS/ path to.

You can also download the latest sidid.cfg file from the SIDId GitHub here: https://github.com/cadaver/sidid



Change Log
=============================
v2.0 rev 3.0 ( changelog on hold beta testing )
- added a couple of things , fiddle and fadiddle with them and you will get the hang of it 
- pushed for time , have fun. all the best.
=============================
v2.0 rev 2 ( changelog on hold beta testing )
- added a couple of things , fiddle and fadiddle with them and you will get the hang of it 
- pushed for time , have fun. all the best.
v1.4d ( changelog on hold beta testing )
-
v1.4c ( changelog on hold beta testing )
-
v1.4b ( changelog on hold beta testing )
- 

-
v1.4a 
- ( AKA : v1.4 internal rev )
- Added on the fly support for live voice muting / filter changes / surround ( some sound issues could remain and will require a manual stop and restart of the playing sid ).
- Code tidy / cleanup / optimized.
- Fixed crash when applying settings or clicking ok with no sid in playlist or no sid playing.
- Full sourcecode available at https://github.com/StronSon


-
v1.3b1 ( beta 1)
- Code tidy / cleanup / optimized.
- Fixed crash when applying settings.
- Added anti click attenuation to attempt to remove the startup click/bump from the start of the played tune.
- Added new surround fx for 2 and 3 sid.
- Updated version info to reflect current release.
- Updated the about dialog to reflect base project authors.
- Various reported bugs fixed from previous versions.
- Feel free to report any bugs you may find @: https://www.un4seen.com/ 
- Many thanks to all involved.


-
v1.3b ( beta )
- Fade in/out was not implementing on the produced sound ( should be fixed now).
- Added anti click atenuation ( allways on will add option to allow a toggle on/off in a future update).
- Made changes to the c64 replay driver for Psids , this should resolve certain issues with tunes not playing/starting.
- Most if not all features are working as is now , please post here with any issues you find.
- https://www.un4seen.com/
- Many thanks to all involved.
- Regard, Malade.

v1.3
- Update to libsidplayfp 2.3.0
- Improved surround effect to add more jangle to those world class jingles ('sid tunes').
- Voice selection disabled when surround sound it active.
- Added saving of voice config when switching between surround and none surround.
- Changed c64 memory pattern to accomodate some sid tunes that do not initialise correctly ( bad rips ), Resulting in corrupt / off key notes being played 
  as a result eg: Shockway Rider played corrupt notes.
- Known Issues: 
	TODO: Fix random crash on switch between surround and normal while player active, 
	      Most likely cause: extra sid forced address using 0xd400 over riding normal mapping, debugs to Time() >> phase debug result my be obfuscated
	      by petite protection/packing of the Xmplay exe. somewhat fixed still working on it.
    TODO : Voice config resets on xmplay close/exit , add saving of user voice config for next start.

v1.2
- Update to libsidplayfp 2.2.2
- Added voice muting option
- Added surround sound option
	FIXED: low volume 8580
v1.1
- Added md5 lookup alternative for files outside of HVSC directory
	Note: Requires stiltxt2md5 to be run in the DOCUMENTS/ folder first 
v1.0
- Added fade-out option
- Added detect music player option
	Note: Requires sidid.cfg to be located in the DOCUMENTS/ folder
- Settings page has been rearranged slightly, it is a little glitchy, I'm okay with it
v0.9
- Lots of tidy ups of silly billy code by Ian :)
- Added a configurable fade-in option to try and hide clicks
- Lock sid model, lock clock speed & 8580 digi-boost now only applies on restart
- Main info sid model/clock speed display will respect the current config
- Stopping or seeking to the start of the file should work as expected
- Added disable seeking option
- Options fixed to disable fields appropriately to better show states
v0.8
- SidDatabase.cpp patched to fix crash processing Songlengths.md5 times
- All STIL entries read instead of sub song specific due to missing records
- Format STIL database code moved to its own function
- 8580 filter strength ranged from 0.0-1.0 instead
- Changed random power delay to use correct value
- Moved songlength database setup & fetch code out to its own function
- Moved STIL database setup code out to its own function
- "Released" converted to UTF8 using XMPlay functions
- Added force default length option
- Added sampling method option
- Added skip short song + threshold options
- 6581 & 8580 filter strength levels are now bars
v0.7
- Removed unused Core option
- Added power delay & random power delay options
v0.6
- A bunch of leaky memory spots fixed by Ian :)
- Compiler reconfigured, libsidplayfp-2.2.1 rebuilt
- 8580 filter strength in and appears to work
- Updated to libsidplayfp-2.2.1
- STIL entry display cleaned up and stable
- STIL titles handled better
- Blank DOCUMENT path bug fixed
- UTF8 of empty values fixed
v0.5
- Several things cleaned up by Ian to be less explosive
- Fixed 0 song length/infinite playback
- SID filter enable option
- 6581 filter strength is in appears to work
- 8580 digi boost option added
- Artist/Title converted to UTF8 using XMPlay functions, seems to work nicely
- Main panel should correctly indicate tune sid model and clock speed now
- Added STIL support
- Turns out relative/full paths were already supported, I just forgot out it worked. (Eg. ../C64Music/DOCUMENTS/) 
     Note the leading / for relative or \ for full paths is important because of reasons
v0.4
- Seeking sort of enabled, it is probably a bit janky
v0.3
- Separating SID files in the play list and playing them individually works now
- Config saving seems to work okay now
v0.2
- Enabled RSID files
- Corrected time shown in the main display info
v0.1
- Initial working version



Big Thanks To
=============================

- Ian Luck ~~~ Code references, logarithmic fade effects and a metric TON of code fixes throughout
- drfiemost (Leandro Nini) ~~~ All the help along the way
- emoon (Daniel Collin) ~~~ Help building libsidplayfp in the first place & code references in HippoPlayer
- hermansr (Roland Hermans) ~~~ Code references in PSID64 adding SIDId support
- cadaver (Lasse Öörni) ~~~ Code references in SIDId and sidid.cfg files
- kode54 (Christopher Snowhill) ~~~ Code references in foo_sid
- z80maniac (Alexey Parfenov) ~~~ Code references in xmp-zxtune
- zbych-r / in_sidplay2 ~~~ Code references in in_sidplay2



Install Instructions
=============================

A. Setup XMPlay
---------------
1. Copy to XMPlay directory (or Plug-in sub-directory).
2. If you want to use STIL/Songlengths and or SIDId download HVSC and point the plugin to the DOCUMENTS folder
3. Add the sidid.cfg file to the DOCUMENTS folder for player detection
	a. Note that though sidid.cfg is included you can use a newer up to date version from https://github.com/cadaver/sidid
