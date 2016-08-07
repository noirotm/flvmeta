# Change Log
All notable changes to this project will be documented in this file.
This project adheres to [Semantic Versioning](http://semver.org/).

## [1.2.0] - 2016-08-08
### Added
- Added dumping of AVC and AAC packet information.
- Added unit tests to the continuous integration system.

### Changed
- Switched to Semver 2.0 as versioning scheme.
- Improved 64-bit and file handling compatibility.
- Improved FLV file checking.

### Removed
- Removed support for the autotools build system.

### Fixed
- Fixed timestamp distance computation in check.
- Fixed installation instructions for CMake.
- Fixed JSON output of non-finite numbers.
- Fixed number printing.

## [1.1.2] - 2013-08-04
### Added
- Added JSON as output format for check reports.
- Added support for the Travis continuous integration platform.

### Removed
- Removed man file from Windows binary packages.

### Fixed
- Fixed automake distribution of the CmakeLists.txt file used
to build the man page.
- Fixed inconsistencies in FLVMeta product spelling.


## [1.1.1] - 2013-05-09
### Changed
- Improved file duration detection.
- Updated copyright notices to 2013.

### Fixed
- Fixed FLVMeta product spelling.


## [1.1.0.1] - 2012-10-18
### Fixed
- Fixed spelling errors in the man page.


## [1.1.0] - 2012-05-03

Beta release. All features should be considered relatively
stable unless explicitely stated otherwise.

### Added
- Added proper command line handling and help.
- Added the possibility to overwrite the input file
when the output file is not specified or when both files
are physically the same.
- Added support for CMake builds in addition to autotools.
It is now the official way to build flvmeta on Windows.
- Added metadata and full file dumping, integrating former
flvdump functionality into flvmeta.
- Added support for XML, YAML, and JSON formats for dumping.
- Added XML schemas describing the various formats used by flvmeta.
- Added a file checking feature.
- Added the possibility to print output file metadata after
a successful update using one of the supported formats.
- Added a feature to insert custom metadata strings while updating.
- Added an option to disable insertion of the onLastSecond event.
- Added an option to preserve existing metadata tags if possible.
- Added an option to fix invalid tags while updating (this is
a highly experimental feature, should be used with caution)
- Added an option to ignore invalid tags as much as possible
instead of exiting at the first error detected.
- Added an option to reset the file timestamps in order to
correctly start at zero, for example if the file has been
incorrectly split by buggy tools.
- Added an option to display informative messages while processing
(not quite exhaustive for the moment).

### Changed
- Changed keyframe index generation so only the first keyframe
for a given timestamp will be indexed. This behaviour can be
overriden with the --all-keyframes/-k option.


## [1.0.11] - 2010-01-25
### Fixed
- Fixed video resolution detection when the first video frame
is not a keyframe.
- Fixed invalid timestamp handling in the case of decreasing timestamps.
- Fixed AVC resolution computation when frame cropping rectangle
is used.
- Fixed handling of files with a non-zero starting timestamp.
- Fixed datasize tag computation so only metadata are taken
into account.


## [1.0.10] - 2009-09-02
### Fixed
- Fixed amf_data_dump numeric format.
- Fixed extended timestamp handling.
- Fixed video resolution detection causing a crash in the case
the video tag data body has less data than required.


## [1.0.9] - 2009-06-23
### Fixed
- Fixed large file support so it will work for files bigger than 4GB.
- Fixed date handling in AMF according to the official spec.
- Fixed extended timestamp handling.
- Fixed a bug causing reading invalid tags to potentially lead to
memory overflow and creation of invalid files.


## [1.0.8] - 2009-05-08
### Added
- Added support for arbitrary large files (2GB+).
- Added support for AVC (H.264) video resolution detection.
- Added support for the Speex codec and rarely met video frame types.

### Fixed
- Fixed a bug where two consecutive tags of different types and
with decreasing timestamps would cause extended timestamps
to be incorrectly used for the next tags.
- Fixed a bug where zero would be used as video height and width
if video resolution could not be detected.
- Fixed a bug causing flvdump to crash after reading invalid tags.
Flvdump now exits after the first invalid tag read.


## [1.0.7] - 2008-09-25
### Added
- Added support for extended timestamps.
Now flvmeta can read and write FLV files longer than 4:39:37,
as well as fix files with incorrect timestamps.
- Added support for all codecs from the official specification.

### Fixed
- Fixed a bug causing flvdump to lose track of tags in case
of invalid metadata.


## [1.0.6] - 2008-05-28
### Fixed
- Fixed a flvdump crash under Linux caused by incorrect string
handling.


## [1.0.5] - 2008-04-03
### Added
- Added support for the AMF NULL type.

### Changed
- Simplified the AMF parser/writer.

### Fixed
- Fixed a bug in the video size detection for VP60.


## [1.0.4] - 2008-01-04
### Changed
- Modified flvdump to make it more tolerant to malformed files.

### Fixed
- Fixed a bug where some data tags wouldn't be written.
- Fixed a date computation bug related to daylight saving.


## [1.0.3] - 2007-10-21
### Fixed
- Fixed major bugs in the AMF parser/writer.
- Fixed a bug in the video size detection for VP6 alpha.
- Fixed minor bugs.


## [1.0.2] - 2007-09-30
### Fixed
- Fixed issues on 64-bits architectures.
- Fixed "times" metadata tag, which was incorrectly written
as "timestamps".
- Fixed audio delay computation.

## [1.0.1] - 2007-09-25
### Fixed
- Fixed a critical bug where file size and offsets would not
be correctly computed if the input file did not have
an onMetaData event.
- Audio related metadata are not added anymore if the FLV file
has no audio data.


## [1.0] - 2007-09-21
This is the first public release.

[1.2.0]: https://github.com/noirotm/flvmeta/releases/tag/v1.2.0
[1.1.2]: https://github.com/noirotm/flvmeta/releases/tag/v1.1.2
[1.1.1]: https://github.com/noirotm/flvmeta/releases/tag/v1.1.1
[1.1.0.1]: https://github.com/noirotm/flvmeta/releases/tag/v1.1.0.1
[1.1.0]: https://github.com/noirotm/flvmeta/releases/tag/v1.1.0
[1.0.11]: https://github.com/noirotm/flvmeta/releases/tag/v1.0.11
[1.0.10]: https://github.com/noirotm/flvmeta/releases/tag/v1.0.10
[1.0.9]: https://github.com/noirotm/flvmeta/releases/tag/v1.0.9
[1.0.8]: https://github.com/noirotm/flvmeta/releases/tag/v1.0.8
[1.0.7]: https://github.com/noirotm/flvmeta/releases/tag/v1.0.7
[1.0.6]: https://github.com/noirotm/flvmeta/releases/tag/v1.0.6
[1.0.5]: https://github.com/noirotm/flvmeta/releases/tag/v1.0.5
[1.0.4]: https://github.com/noirotm/flvmeta/releases/tag/v1.0.4
[1.0.3]: https://github.com/noirotm/flvmeta/releases/tag/v1.0.3
[1.0.2]: https://github.com/noirotm/flvmeta/releases/tag/v1.0.2
[1.0.1]: https://github.com/noirotm/flvmeta/releases/tag/v1.0.1
[1.0]: https://github.com/noirotm/flvmeta/releases/tag/v1.0
