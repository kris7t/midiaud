midiaud
=======

`midiaud` is a simple [MIDI][midi] player powered by [libsmf][libsmf]
and [JACK][jack]. `midiaud` is not a synthesizer: you can freely
connect its MIDI output to any [SoundFont][soundfont] player or VST
host you can imagine using JACK's routing capabilities. It also has no
GUI on its own but synchronizes with JACK's transport so that you can
use it in conjunction with any other playback software. More
concretely, you can run many instances of `midiaud` if you require the
playback of multiple MIDI files (i.e. more than 16 channels) with full
sample-accurate transport synchronization, even on multiple machines
at the same time.

Requirements
------------

`midiaud` requires [libsmf][libsmf], [JACK][jack] and [Boost][boost]
to build and run. Additionally, you will need a C++11 compatible
compiler.

Building and Installing
-----------------------

`midiaud` uses [waf][waf] as build tool. Run

	./waf configure

with your configuration options of choice, then run

	./waf

to build. Currently, no installation script is provided. The binary
`midiaud` resides in the directory `build/src`.

Usage
-----

For a detailed description of available command-line options, run

	midiaud --help

You could use e.g. [QJackCtl][qjackctl] to connect `midiaud` to you
synthesizer and control the transport if you need a graphical
tool. Sending `SIGINT` to `midiaud` will cause it to terminate
gracefully.

Limitations and Todo
--------------------

* MIDI controls should change value when the transport is
  repositioned. More concretely, messages up to the point where the
  new playing position is should be analyzed and if the value of a
  control differs from the value before the repositioning, a control
  change event should be emitted.
* The same thing as above should be done program changes.
* The same thing as above should be done to notes. More concretely, if
  there is note being held at the new playing position, a note on
  message for it should be emitted. Probably hearing the brief attack
  of the note when it is not indented is a lesser evil than not
  hearing the note at all, especially for long, sustained pads.
* For auditioning MIDI files (e.g. the output of
  [LilyPond][lilypond]), either automatic (when a command-line option
  is specified) or manual (e.g. by sending `SIGUSR1`) reloading of the
  MIDI file would be nice to have. This reloading should not cause
  music to glitch or stop.

License
-------

Copyright (c) 2013, Kristóf Marussy

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the
   distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

[midi]: http://en.wikipedia.org/wiki/MIDI "MIDI from Wikipedia, the free encyclopedia"
[libsmf]: http://sourceforge.net/projects/libsmf/ "Standard MIDI File format library"
[jack]: http://jackaudio.org/ "JACK Audio Connection Kit"
[soundfont]: http://en.wikipedia.org/wiki/SoundFont "SoundFont from Wikipedia, the free encyclopedia"
[boost]: http://www.boost.org/ "Boost C++ Libraries"
[waf]: http://code.google.com/p/waf/ "waf - The meta build system"
[qjackctl]: http://qjackctl.sourceforge.net/ "QjackCtl JACK Audio Connection Kit - Qt GUI Interface"
[lilypond]: http://lilypond.org/ "LilyPond music engraving program"
