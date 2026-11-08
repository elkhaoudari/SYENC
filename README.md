# symenc

[![ci](https://github.com/elkhaoudari/symenc/actions/workflows/ci.yml/badge.svg)](https://github.com/elkhaoudari/symenc/actions/workflows/ci.yml)
[![license: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

Reference implementation of the **Symbolic Encoding Protocol v0.1.1** (frozen).
C++17, no third-party dependencies. The full specification is in
[docs/SPEC.md](docs/SPEC.md). The source files contain no comments; this file
holds the behavior reference instead.

## What this is, and is not

A small, fully specified, reversible text encoding for ASCII data, built as a
spec-first engineering exercise: frozen grammar, decoder state machine, error
codes with exact offsets, test vectors, and an implementation checked against
them.

- It is **not encryption**: the mapping is public and keyless.
- It is **not compression**: output is about 72 times larger on average (3 to
  153 bytes per input byte).
- Input is limited to ASCII (0x00 to 0x7F).

## Build and test

    cmake -S . -B build
    cmake --build build
    ctest --test-dir build --output-on-failure

Strict build with AddressSanitizer and UBSan:

    cmake -S . -B build-san -DSYMENC_SANITIZE=ON
    cmake --build build-san
    ctest --test-dir build-san --output-on-failure

Options: `SYMENC_WERROR` (ON), `SYMENC_SANITIZE` (OFF), `SYMENC_BUILD_TESTS` (ON).
The default build type is RelWithDebInfo.

## Usage

    symenc encode IN OUT
    symenc decode IN OUT
    symenc --version | --help

`IN` and `OUT` may be `-` for stdin and stdout. Identical input and output
paths are refused. File output is written to a temporary file next to the
destination and renamed over it.

| Exit code | Meaning |
|---|---|
| 0 | success |
| 1 | protocol error, reported as `symenc: E_BAD_TOKEN at byte offset 3` |
| 2 | usage error |
| 3 | I/O error (missing file, unwritable destination, failing stdout, out of memory) |

## Format

| Token | Bytes | Meaning |
|---|---|---|
| `€` | E2 82 AC | +1 |
| `JIG` | 4A 49 47 | +50 |
| `SAW` | 53 41 57 | end of group |

A group encodes one byte (0 to 127): JIG x j, then € x e, then SAW, with
N = 50j + e, j <= 2, e <= 49, N <= 127. Whitespace (space, tab, LF, CR) is
accepted between tokens when decoding and never produced when encoding.

## Behavior reference

- **Errors** are returned values (`Result<T>` = `std::variant<T, Error>`), never
  exceptions. Codes: `E_NONASCII_INPUT` (encoder only), `E_BAD_TOKEN`,
  `E_ORDER`, `E_NONCANONICAL`, `E_RANGE`, `E_TRUNCATED`.
- **Offsets** are zero-based raw byte offsets into the encoded text (the source
  for `E_NONASCII_INPUT`), counting whitespace. Token errors point at the first
  byte of the offending token. `E_TRUNCATED` reports the input length.
- **Precedence**: the first violation in one left-to-right scan is reported. A
  token is checked lexically before the state machine. Within one token the
  order is `E_ORDER`, `E_NONCANONICAL`, `E_RANGE`. Counts and range are checked
  eagerly, at the token that makes the group impossible.
- **End of input** inside a token or a group is `E_TRUNCATED`. A token prefix
  followed by a non-matching byte is `E_BAD_TOKEN`.
- **No partial output**: conversion completes in memory before anything is
  written. This covers protocol failures; a physical write failure on stdout
  cannot guarantee zero bytes written and exits with code 3.
- **Not protocol errors**: `std::length_error` (encoded size not representable),
  `std::bad_alloc`, and `IoError` for file and stream failures.
- `tokenizer.next_token(text, pos)` leaves `pos` unchanged on failure and treats
  a `pos` beyond the end as the end.

## Layout

    include/symenc/   public headers (error, protocol, tokenizer, codec, io, cli, version)
    src/              symenc_core library
    src/cli/          command-line logic (symenc_cli) and main()
    tests/            one test executable per module, tests/support = harness,
                      tests/data = sample sources, tests/cli_e2e.cmake = process-level test
    cmake/            warning and sanitizer configuration

## Testing

126 unit tests plus a process-level end-to-end test: spec vectors, an exhaustive
sweep of all 128 values, seeded random round trips, a differential fuzz against
a second decoder, binary and atomic file handling, and CLI exit codes. Verified
with GCC 13 and Clang 18 on Linux, including AddressSanitizer and UBSan. The
macOS and Windows CI jobs are informational until their first green run.

## License

MIT. See [LICENSE](LICENSE).
