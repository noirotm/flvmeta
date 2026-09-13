# Working on FLVMeta

## Scope and project map

These instructions apply throughout this repository. FLVMeta is a portable C
command-line utility for inspecting, checking, and updating FLV metadata. Preserve
its ability to process large files with a two-pass approach and bounded memory
usage rather than loading whole media files into memory.

- `src/flvmeta.c`, `src/flvmeta.h`: CLI options, command dispatch, and exit codes.
- `src/flv.c`, `src/amf.c`: FLV stream handling and AMF metadata serialization.
- `src/types.*`, `src/bitstream.*`, `src/avc.*`: binary types, endian conversion,
  bit reading, and codec parsing.
- `src/info.*`, `src/update.*`: metadata computation and file rewriting.
- `src/check.*`, `src/dump*`: validity reports and raw, XML, JSON, and YAML output.
- `tests/check_amf.c`, `tests/check_flv.c`: Unity test suites;
  `tests/check_flvmeta.c` is the test runner.
- `tests/regressions/issue-N/`: CLI regression tests grouped by GitHub issue;
  `tests/regressions/support/` provides shared fixture and CLI helpers.
- `man/flvmeta.1.md`: CLI manual source. `schemas/` contains XML schemas and examples.
- `CMakeLists.txt`, `src/CMakeLists.txt`, `tests/CMakeLists.txt`,
  `config-cmake.h.in`, `cmake/modules/`: build configuration and portability checks.

Read the relevant source and tests before editing. Keep changes focused, preserve
unrelated working-tree changes, and avoid incidental reformatting or dependency
updates.

## Build and test

The root CMake configuration requires CMake 3.11 or newer and a C compiler.
Treat the CMake files and `.github/workflows/build.yml` as authoritative when
older examples in `INSTALL.md` disagree. CI builds and tests Release on Linux,
macOS, and Windows.

Use an out-of-source build in a dedicated directory. The following commands run
from the repository root, including with CMake 3.11:

```sh
cmake -E make_directory build-agent
cd build-agent
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --config Debug
ctest -C Debug --output-on-failure
cd ..
```

Choose another build directory if `build-agent` already holds a different
toolchain or configuration. For Visual Studio or Xcode, `--config Debug` and
`ctest -C Debug` select the configuration; `CMAKE_BUILD_TYPE` applies to
single-configuration generators. To reproduce CI, use Release in place of Debug.

Bundled libyaml is the default. Test `-DFLVMETA_USE_SYSTEM_LIBYAML=ON` in a separate
build when changing YAML integration and a system libyaml is available. Pandoc
is optional for the executable and tests; when found, it generates the manual
through the `man` target.

For C or build changes, build and run CTest. Add focused regression tests for
behavior changes and bug fixes. Documentation-only edits need no compilation;
check referenced paths, commands, and the diff instead. Report what was actually
tested and any toolchain or dependency limitations; do not claim unrun platform
checks passed.

## C conventions and portability

- Follow neighboring code: generally four-space indentation, same-line opening
  braces, snake_case functions and variables, and uppercase macros. Preserve
  existing license headers.
- Declare all local variables at the beginning of each function, before any
  executable statements. Do not interleave declarations with statements or
  declare variables in loop initializers or nested blocks.
- Keep implementation in C and avoid requiring a newer language standard or
  platform-specific extension without an explicit need. The build does not
  currently select a C standard.
- Reuse the types, byte-order helpers, and file-offset abstractions in
  `src/types.h`. Do not assume native integer widths, host byte order, structure
  packing, alignment, or that `long` can hold a file offset.
- Preserve Windows/MSVC and Unix support. Add feature checks in CMake and their
  corresponding definitions in `config-cmake.h.in`; never edit generated
  `config.h` as the source of a fix.
- Check allocations and I/O results. Make ownership and cleanup clear on both
  success and failure, following existing error-return conventions.
- Keep file-local helpers `static`; update declarations and callers together
  when changing an interface.

## Binary data and compatibility

- Treat FLV/AMF input as untrusted. Validate lengths, counts, offsets, and
  remaining input before reading, allocating, seeking, or performing arithmetic.
  Check overflow and signed/unsigned conversions before using computed sizes.
- Handle truncated input and malformed tags without out-of-bounds access,
  unbounded recursion, leaks, or loops that stop making progress.
- Preserve 24-bit field handling, extended 32-bit timestamps, large-file offsets,
  and stream-state invariants. Test boundaries relevant to the change.
- Preserve CLI options, defaults, exit statuses, stdout/stderr roles, and emitted
  metadata/report formats unless the requested change requires otherwise.
  Keep machine-readable output valid, including escaping and numeric values.
- For rewrite changes, verify resulting tag sizes, keyframe offsets, timestamps,
  and metadata. Use disposable fixtures and explicit output paths for manual
  checks; never overwrite a user's original media as a test.
- Update CLI help and `man/flvmeta.1.md` together for user-visible option changes.
  Review `schemas/` when changing XML structure.

## Tests and change hygiene

- Extend the existing Unity suites with small, deterministic cases and register
  them in the relevant `run_*_tests` function. New suites also need registration
  in `tests/check_flvmeta.c` and sources in `tests/CMakeLists.txt`.
- Put issue-specific CLI regressions in `tests/regressions/issue-N/`, with a
  GitHub issue link and the standard `fixture.c`, `test.cmake`, and
  `CMakeLists.txt` filenames. Register them with `add_flvmeta_regression(N)` and
  add the issue directory to `tests/regressions/CMakeLists.txt`.
- Reuse the shared fixture driver and CLI helpers rather than duplicating FLV
  framing, temporary-file handling, or process execution. Keep issue-specific
  payloads and behavior assertions in the issue directory. See
  [the regression testing guide](tests/regressions/README.md) for the helper
  interfaces and registration details.
- Assert exact expected exit codes and relevant diagnostics or output behavior.
  A crash, timeout, or unrelated error must not count as successful rejection.
  Run supplied reproducer files when available, using read-only checks or
  explicit disposable output paths; never overwrite the original samples.
- Prefer constructed byte sequences and temporary files over large media
  fixtures. Follow the existing `FLVMETA_TEST_TMP_DIR` and process-specific path
  pattern, and clean up files and allocated AMF data.
- The Unity test executable compiles AMF, FLV, and type code directly; it does
  not exercise every CLI, rewrite, or output path. Use the CLI regression
  framework for bugs in those paths. From the build directory, run
  `ctest -C Debug -L regression --output-on-failure` for all regressions, or
  `-L issue-N` for one issue. The full CTest run includes both unit and regression
  tests and remains required for C or build changes.
- For memory or parser changes, use address/undefined-behavior sanitizers when
  supported by the available compiler. Keep such flags local to the test build.
  Report which sanitizers actually ran; running a sample named for UBSan under
  ASan does not establish UBSan coverage.
- Treat `src/libyaml/`, `src/compat/`, and `tests/unity*` as bundled third-party
  code. Change them only when necessary for the task, preserve their notices,
  and explain any local patch.
- Do not commit generated build files, binaries, temporary media, or generated
  manual pages. Avoid editing IDE-specific configuration for general fixes.
- Before finishing, inspect `git diff --check` and the final diff. Summarize the
  behavior changed, validation performed, and any remaining limitations.
