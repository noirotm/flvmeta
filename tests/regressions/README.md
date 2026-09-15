# CLI regression tests

Each `issue-N` directory contains its GitHub issue link, fixture payload, and
behavior assertions. Unit tests remain in the parent `tests` directory.

Every issue directory uses the same filenames:

- `CMakeLists.txt`: issue link, registration, and any extra dependencies.
- `fixture.c`: the issue's input generator.
- `test.cmake`: commands and behavior assertions.

Shared support:

- `support/fixture.c` provides the generator entry point, process-specific
  temporary paths, FLV header and metadata tag framing, and I/O cleanup.
  Each generator implements `write_flv_fixture()` from `support/fixture.h`.
  Payload bytes are constructed independently of the production AMF writer.
  `fixture_header()` writes a header with selected stream flags and
  `fixture_tag()` writes a complete tag at timestamp zero for audio/video cases.
  `fixture_tag_at()` accepts an explicit 32-bit timestamp in milliseconds.
- `support/cli.cmake` provides `generate_fixture()`,
  `assert_flvmeta_exit_code()`, and `remove_fixtures()`. Exit codes are compared
  exactly; commands have a timeout. Failed cases retain their files.
- `add_flvmeta_regression(issue)` builds `fixture.c` as `issue_N_fixture` and
  registers `test.cmake` as CTest entry `issue_N`, with `regression` and
  `issue-N` labels.

To add a case, add `add_subdirectory(issue-N)` to this directory's
`CMakeLists.txt` and call `add_flvmeta_regression(N)` in the issue's own
`CMakeLists.txt`. Include `../support/cli.cmake` from `test.cmake`.
`generate_fixture(path args...)` passes the case arguments to the
generator and returns its output path. The issue script owns its assertions
and calls `remove_fixtures()` after they pass.

From a configured build directory, run all regression tests with
`ctest -C Debug -L regression --output-on-failure`, or use `-L issue-29` to
select one issue.
