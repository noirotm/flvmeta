# FLVmeta - FLV Metadata Editor

flvmeta is a command-line utility aimed at manipulating Adobe(tm) Flash
Video files (FLV), through several commands, only one of which can be used for
each invocation of the program.

It possesses the ability to compute and inject a variety of values in the
_onMetaData_ event tag, including keyframe indices used by most video players to
allow random-access seeking, notably for HTTP pseudo-streamed files via a 
server-side module, by having the client send the file offset looked up for the
nearest desired keyframe.  
Tools such as flvmeta must be used in the case the initial encoding process
is unable to inject those metadata.

It can also optionnally inject the _onLastSecond_ event, used to signal the end
of playback, for example to revert the player software to a 'stopped' state.

flvmeta also has the ability to dump metadata and full file information to
standard output, in a variety of textual output formats, including XML, YAML,
and JSON.

Finally, the program can analyze FLV files to detect potential problems and
errors, and generate a textual report in a raw format, or in XML.
It has the ability to detect more than a hundred problems, going from harmless
to potentially unplayable, using a few real world encountered issues.

flvmeta can operate on arbitrarily large files, and can handle FLV files
using extended (32-bit) timestamps.
It can guess video frame dimensions for all known video codecs supported by the
official FLV specification.

Its memory usage remains minimal, as it uses a two-pass reading algorithm which
permits the computation of all necessary tags without loading anything more than
the file's tags headers in memory.

The FLVmeta source package includes and uses the following software:

* the libyaml YAML parser and emitter ([http://pyyaml.org/wiki/LibYAML](http://pyyaml.org/wiki/LibYAML "LibYAML")).


FLVmeta is provided "as is" with no warranty.  The exact terms
under which you may use and (re)distribute this program are detailed
in the GNU General Public License, in the file COPYING.

See the files AUTHORS and THANKS for a list of authors and other contributors.

See the file INSTALL for compilation and installation instructions.

See the file NEWS for a description of major changes in this release.

See the file TODO for ideas on how you could help us improve FLVmeta.
