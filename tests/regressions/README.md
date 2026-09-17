# CLI regression tests

Each `issue-N` directory contains its GitHub issue link, fixture payload, and
behavior assertions. Unit tests remain in the parent `tests` directory.
Regressions without a GitHub issue use a descriptive directory name, such as
`video-body-length`.

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
  Output assertions are `assert_contains(actual text)`,
  `assert_not_contains(actual text)`, `assert_equal(actual expected)`, and
  `assert_matches(actual regex)`.
- `add_flvmeta_regression(issue)` builds `fixture.c` as `issue_N_fixture` and
  registers `test.cmake` as CTest entry `issue_N`, with `regression` and
  `issue-N` labels.

To add a case, add `add_subdirectory(issue-N)` to this directory's
`CMakeLists.txt` and call `add_flvmeta_regression(N)` in the issue's own
`CMakeLists.txt`. Include `../support/cli.cmake` from `test.cmake`.
`generate_fixture(path args...)` passes the case arguments to the
generator and returns its output path. The issue script owns its assertions
and calls `remove_fixtures()` after they pass.

## Writing readable cases

Set `TEST_CONTEXT` to the issue, case, and operation before running a command
or checking its output. Helpers include this context in failure messages and
inherit it through nested CMake functions. Keep the actual CLI arguments and
expected exit code visible in each test:

```cmake
set(TEST_CONTEXT "issue-32 / maximum / check")
assert_flvmeta_exit_code(9 check_output --check "${input}")
assert_contains("${check_output}" "W80062")
assert_contains("${check_output}" "audiodelay should be 4294967.295, got 42")
```

Use literal assertions for fixed diagnostics; punctuation should not require
regex escaping. Use `assert_matches` when variable whitespace or an end-of-output
anchor matters. CMake bracket arguments keep quotes and backslashes readable:

```cmake
assert_matches("${metadata_output}" [["width" *: *320]])
```

Use names such as `strict-array` for fixture arguments, and include units in
numeric names such as `audio_timestamp_ms` and `expected_delay_seconds`.
Name captured output after its operation: `check_output`, `update_output`, or
`metadata_output`. Save regex captures before calling other helpers, which may
perform their own matches.

Group byte arrays by encoded field, with comments for types, lengths, values,
and terminators. Keep bytes independent of the production serializer, including
deliberately invalid values. Short-circuit I/O chains are fine; use a `success`
variable when resources must be freed before returning.

Comments should explain choices that would otherwise be surprising: why a
fixture contains an incorrect value, why exit 9 is expected, or which diagnostic
proves the intended code path was reached. An error exit alone is insufficient:
an unrelated rejection must not pass a regression. These tests invoke the real
executable because the unit test target does not cover all CLI paths.

Call `remove_fixtures()` only after all assertions pass. This leaves the input
and any rewritten output available for investigation when a test fails.

## Running tests

From a configured build directory, run all regression tests with
`ctest -C Debug -L regression --output-on-failure`, or use `-L issue-29` to
select one issue.
