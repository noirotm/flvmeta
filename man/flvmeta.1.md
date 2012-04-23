% flvmeta(1) flvmeta user manual
% 
% April 2012

# NAME

flvmeta - manipulate or extract metadata in Adobe Flash Video files

# SYNOPSIS

**flvmeta** *INPUT_FILE*  
**flvmeta** *INPUT_FILE* *OUTPUT_FILE*  
**flvmeta** `-D`|`--dump` [*options*] *INPUT_FILE*  
**flvmeta** `-F`|`--full-dump` [*options*] *INPUT_FILE*  
**flvmeta** `-C`|`--check` [*options*] *INPUT_FILE*  
**flvmeta** `-U`|`--update` [*options*] *INPUT_FILE* [*OUTPUT_FILE*]

# DESCRIPTION

**flvmeta** is a command-line utility aimed at manipulating Adobe(tm) Flash
Video files (FLV), through several commands, only one of which can be used for
each invocation of the program.

It possesses the ability to compute and inject a variety of values in the
_onMetaData_ event tag, including keyframe indices used by most video players to
allow random-access seeking, notably for HTTP pseudo-streamed files via a 
server-side module, by having the client send the file offset looked up for the
nearest desired keyframe.  
Tools such as **flvmeta** must be used in the case the initial encoding process
is unable to inject those metadata.

It can also optionnally inject the _onLastSecond_ event, used to signal the end
of playback, for example to revert the player software to a 'stopped' state.

**flvmeta** also has the ability to dump metadata and full file information to
standard output, in a variety of textual output formats, including XML, YAML,
and JSON.

Finally, the program can analyze FLV files to detect potential problems and
errors, and generate a textual report in a raw format, or in XML.
It has the ability to detect more than a hundred problems, going from harmless
to potentially unplayable, using a few real world encountered issues.
This analysis can also determine and display the minimal Flash Player version
which can be used to correctly play a given file, as well as codec information.

**flvmeta** can operate on arbitrarily large files, and can handle FLV files
using extended (32-bit) timestamps.
It can guess video frame dimensions for all known video codecs supported by the
official FLV specification.

Its memory usage remains minimal, as it uses a two-pass reading algorithm which
permits the computation of all necessary tags without loading anything more than
the file's tags headers in memory.
Only the dumping of JSON data can consume more memory, since the bundled `mjson`
library constructs a data tree in memory before outputting the formatted data.

# COMMANDS

Only one command can be specified for an invocation of **flvmeta**. The chosen
command determines the mode of execution of the program.

By default, if no command is specified, **flvmeta** will implicitely choose the
command to use according to the presence of *INPUT_FILE* and *OUTPUT_FILE*.

If only *INPUT_FILE* is present, the `--dump` command will be executed.

If both *INPUT_FILE* and *OUTPUT_FILE* are present, the `--update` command
will be executed.

Here is a list of the supported commands:

## -D, \--dump

Dump a textual representation of the first _onMetaData_ tag found in
*INPUT_FILE* to standard output. The default format is XML, unless
specified otherwise.  
It is also possible to specify another event via the `--event` option,
such as _onLastSecond_.

## -F, \--full-dump

Dump a textual representation of the whole contents of *INPUT_FILE* to
standard output. The default format is XML, unless specified otherwise.
  
## -C, \--check

Print a report to standard output listing warnings and errors detected in
*INPUT_FILE*, as well as potential incompatibilities, and informations about
the codecs used in the file. The exit code will be set to a non-zero value
if there is at least one error in the file.

The output format can either be plain text or XML using the `--xml` option.
It can also be disabled altogether using the `--quiet` option if you are
only interested in the exit status.

Messages are divided into four specific levels of increasing importance:

* **info**: informational messages that do not pertain to the file validity  
* **warning**: messages that inform of oddities to the flv format but that
  might not hamper file reading or playability, this is the default level  
* **error**: messages that inform of errors that might render the file
  impossible to play or stream correctly  
* **fatal**: messages that inform of errors that make further file reading
  impossible therefore ending parsing completely

The `--level` option allows to limit the display of messages to a minimum
level among those, for example if the user is only interested in error
messages and above.

Each message or message template presented to the user is identified by a
specific code of the following format:

`[level][topic][id]`

* **level** is an upper-case letter that can be either I, W, E, F according to
  the aforementioned message levels  
* **topic** is a two-digit integer representing the general topic of the
  message  
* **id** is a unique three-digit identifier for the message, or message
  template if parameterized
  
Messages can be related to the following topics :

* **10** general flv file format  
* **11** file header  
* **12** previous tag size  
* **20** tag format  
* **30** tag types  
* **40** timestamps  
* **50** audio data  
* **51** audio codecs  
* **60** video data  
* **61** video codecs  
* **70** metadata  
* **80** AMF data  
* **81** keyframes  
* **82** cue points

For example, <W51050> represents a Warning in topic 51 with the id 050,
which represents a warning message related to audio codecs, in that case to
signal that an audio tag has an unknown codec.
    
## -U, \--update

Update the given input file by inserting a computed _onMetaData_ tag. If
*OUTPUT_FILE* is specified, it will be created or overwritten instead and
the input file will not be modified. If the original file is to be updated,
a temporary file will be created in the default temp directory of the
platform, and it will be copied over the original file at the end of
the operation. This is due to the fact that the output file is written
while the original file is being read due to the two-pass method.

The computed metadata contains among other data full keyframe informations,
in order to allow HTTP pseudo-streaming and random-access seeking in the
file.

By default, an _onLastSecond_ tag will be inserted, unless the
`--no-last-second` option is specified.

Normally overwritten by the update process, the existing metadata found in
the input file can be preserved by the `--preserve` option.

It is also possible to insert custom string values with the `--add` option,
which can be specified multiple times.

By default, the update operation is performed without output, unless the
`--verbose` option is specified, or the `--print-metadata` is used to
print the newly written metadata to the standard output.

# OPTIONS

## DUMP

-d *FORMAT*, \--dump-format=*FORMAT*
:   specify dump format where *FORMAT* is 'xml' (default), 'json', 'raw', or
    'yaml'. Also applicable for the **\--full-dump** command.

-j, \--json
:   equivalent to **\--dump-format=json**

-r, \--raw
:   equivalent to **\--dump-format=raw**

-x, \--xml
:   equivalent to **\--dump-format=xml**

-y, \--yaml
:   equivalent to **\--dump-format=yaml**

-e *EVENT*, \--event=*EVENT*
:   specify the event to dump instead of _onMetaData_, for example
    _onLastSecond_.

## CHECK

-l *LEVEL*, \--level=*LEVEL*
:   print only messages where level is at least *LEVEL*. The levels are, by
    ascending importance, 'info', 'warning' (default), 'error', or 'fatal'.

-q, \--quiet
:   do not print messages, only return the status code

-x, \--xml
:   generate an XML report instead of the default 'compiler-friendly' text

## UPDATE

-m, \--print-metadata
:   print metadata to stdout after update using  the format specified by
    the **\--format** option

-a *NAME=VALUE*, \--add=*NAME=VALUE*
:   add a metadata string value to the output file. The name/value pair will be
    appended at the end of the _onMetaData_ tag.

-s, \--no-lastsecond
:   do not create the *onLastSecond* tag

-p, \--preserve
:   preserve input file existing *onMetadata* tags

-f, \--fix
:   fix invalid tags from the input file

-i, \--ignore
:   ignore invalid tags from the input file (the default behaviour is to stop
    the update process with an error)

-t, \--reset-timestamps
:   reset timestamps so *OUTPUT_FILE* starts at zero. This has been added
    because some FLV files are produced by cutting bigger files, and the
    software doing the cutting does not resets the timestamps as required
    by the standard, which can cause playback issues.

-k, --all-keyframes
:   index all keyframe tags, including duplicate timestamps

## GENERAL

-v, \--verbose
:   display informative messages

-V, \--version
:   print version information and exit

-h, \--help
:   display help on the program usage and exit

# FORMATS

# EXAMPLES

# EXIT STATUS

# BUGS

**flvmeta** does not support encrypted FLV files yet.

# AUTHOR

# COPYRIGHT

# CONTACT
