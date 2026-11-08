# SYENC — Symbolic Encoding Protocol

<p align="center">
  <strong>A deterministic, reversible symbolic encoding protocol.</strong>
  <br>
  A weighted token system for representing 7-bit ASCII data.
</p>

<p align="center">
  <img src="https://img.shields.io/badge/version-0.1.1-blue" alt="Version">
  <img src="https://img.shields.io/badge/language-C%2B%2B17-blue" alt="C++17">
  <img src="https://img.shields.io/badge/license-MIT-green" alt="MIT License">
  <a href="https://github.com/elkhaoudari/symenc/actions/workflows/ci.yml">
    <img src="https://github.com/elkhaoudari/symenc/actions/workflows/ci.yml/badge.svg" alt="CI">
  </a>
</p>

---

## Overview

**SYENC (Symbolic Encoding Protocol)** is a deterministic and reversible encoding protocol that transforms 7-bit ASCII data into a structured symbolic representation using a weighted token system.

The protocol is built around three symbolic tokens:

| Token | UTF-8 Representation | Weight / Function |
|---|---|---:|
| `JIG` | `4A 49 47` | +50 |
| `€` | `E2 82 AC` | +1 |
| `SAW` | `53 41 57` | Group delimiter |

Each input byte is represented by a canonical sequence of these tokens. The decoder reconstructs the original byte using the same mathematical rules.

SYENC is designed as a language-independent protocol, with this repository providing its C++17 reference implementation.

**Current specification:** v0.1.1 — Frozen.

**Full protocol specification:** [`docs/SPEC.md`](docs/SPEC.md).

## Key Features

- Deterministic and reversible encoding.
- Language-independent protocol specification.
- Weighted symbolic representation.
- Strict token recognition and grammar validation.
- Six explicitly defined protocol error categories.
- Zero-based byte-offset error reporting.
- Binary file input and output.
- Command-line interface.
- Automated unit, integration, and round-trip testing.
- No third-party runtime dependencies.

## Mathematical Foundation

SYENC uses integer division and modulo arithmetic to decompose each supported byte value into weighted components.

For an input byte \(N\), the encoder calculates:

\[
j = \left\lfloor \frac{N}{50} \right\rfloor
\]

\[
e = N \bmod 50
\]

Where:

- \(N\): Original byte value.
- \(j\): Number of `JIG` tokens.
- \(e\): Number of `€` tokens.

The resulting symbolic group is:

\[
E(N) = JIG^j \cdot EURO^e \cdot SAW
\]

The decoder reconstructs the original value:

\[
\boxed{N = 50j + e}
\]

### Supported Range

The current version supports:

\[
N \in \{0,1,\ldots,127\}
\]

With the following constraints:

\[
j \in \{0,1,2\}
\]

\[
0 \leq e \leq 49
\]

Additionally:

\[
j=2 \implies e\leq27
\]

These constraints guarantee that every encoded group represents exactly one valid ASCII byte.

### Reversibility

The fundamental correctness property is:

\[
\boxed{D(E(N))=N}
\]

For every supported input byte:

\[
N\in[0,127]
\]

The protocol is deterministic: identical input bytes always produce identical encoded output.

## Encoding Example

Consider the ASCII character `l`.

Its decimal ASCII value is:

\[
N=108
\]

Calculate the weighted components:

\[
j=\left\lfloor\frac{108}{50}\right\rfloor=2
\]

\[
e=108\bmod50=8
\]

The resulting SYENC representation is:

```text
JIGJIG€€€€€€€€SAW
```

Decoding:

\[
N=(2\times50)+(8\times1)=108
\]

Therefore:

```text
108 → l
```

## Encoding Efficiency

SYENC uses a weighted representation rather than a naive one-unit-per-value representation.

For example, representing the value 108 using naive unary encoding requires 108 repeated marks.

SYENC represents the same value using:

- 2 `JIG` tokens.
- 8 `€` tokens.
- 1 `SAW` delimiter.

This results in 11 symbolic tokens instead of 108 repeated units.

This comparison concerns symbolic unit count, not encoded byte size.

### Storage Characteristics

Each token occupies exactly three bytes in the encoded representation.

The minimum encoded group size is:

\[
3\text{ bytes}
\]

The maximum encoded group size is:

\[
(2+49+1)\times3=156\text{ bytes}
\]

**Correction:** The maximum for the supported range is actually 153 bytes, because when \(j=2\), \(e\leq27\). Thus:

\[
(2+27+1)\times3=\boxed{90\text{ bytes}}
\]

The actual maximum occurs at \(N=99\):

\[
(1+49+1)\times3=\boxed{153\text{ bytes}}
\]

Across all 128 supported ASCII values, the average encoded size is:

\[
\boxed{71.765625\text{ bytes per input byte}}
\]

SYENC is therefore not a compression algorithm and is not designed to outperform Base64, Base85, or hexadecimal in storage efficiency.

Its purpose is to provide a distinct symbolic representation with explicit grammar and deterministic decoding.

## Protocol Behavior

### Token Grammar

A valid group consists of:

1. Zero to two `JIG` tokens.
2. Zero to forty-nine `€` tokens.
3. Exactly one `SAW` delimiter.

The special range constraint for two `JIG` tokens must also be respected.

### Whitespace

The decoder accepts the following whitespace characters between tokens:

- Space (`SP`)
- Horizontal tab (`TAB`)
- Line feed (`LF`)
- Carriage return (`CR`)

The encoder does not generate whitespace.

### Error Handling

The protocol defines six error categories:

| Error Code | Description |
|---|---|
| `E_NONASCII_INPUT` | Encoder received a byte greater than 127. |
| `E_BAD_TOKEN` | Unknown or malformed token. |
| `E_ORDER` | Token sequence violates the required order. |
| `E_NONCANONICAL` | Token count violates canonical representation rules. |
| `E_RANGE` | Decoded value exceeds the supported ASCII range. |
| `E_TRUNCATED` | Input ends inside a token or incomplete group. |

Errors include zero-based byte offsets identifying the relevant position in the input.

The decoder validates the symbolic representation before producing the final decoded result.

For additional implementation details, see [`docs/SPEC.md`](docs/SPEC.md).

## C++17 Implementation

This repository provides a reference implementation written in C++17.

### Architecture

```text
symenc/
│
├── include/
│   └── symenc/
│       ├── cli.hpp
│       ├── codec.hpp
│       ├── error.hpp
│       ├── io.hpp
│       ├── protocol.hpp
│       ├── tokenizer.hpp
│       └── version.hpp
│
├── src/
│   ├── cli/
│   ├── decoder.cpp
│   ├── encoder.cpp
│   ├── error.cpp
│   ├── io.cpp
│   ├── tokenizer.cpp
│   └── version.cpp
│
├── tests/
│
├── docs/
│   └── SPEC.md
│
├── cmake/
├── CMakeLists.txt
├── README.md
└── LICENSE
```

### Core Components

| Component | Responsibility |
|---|---|
| Encoder | Converts supported bytes into SYENC tokens. |
| Decoder | Reconstructs original bytes from valid SYENC data. |
| Tokenizer | Recognizes protocol tokens at the byte level. |
| Error Handling | Reports structured protocol errors and offsets. |
| File I/O | Handles binary input and output. |
| CLI | Exposes encoding and decoding operations. |

## Installation and Build

### Requirements

- C++17-compatible compiler.
- CMake 3.16 or later.
- Standard C++ library.

No third-party runtime dependencies are required.

### Clone the Repository

```bash
git clone https://github.com/elkhaoudari/symenc.git
cd symenc
```

### Configure

```bash
cmake -S . -B build
```

### Build

```bash
cmake --build build
```

### Run Tests

```bash
ctest --test-dir build --output-on-failure
```

### Sanitizer Build

To enable AddressSanitizer and UndefinedBehaviorSanitizer:

```bash
cmake -S . -B build-san -DSYMENC_SANITIZE=ON
cmake --build build-san
ctest --test-dir build-san --output-on-failure
```

### CMake Options

| Option | Default | Description |
|---|---|---|
| `SYMENC_WERROR` | ON | Treat compiler warnings as errors. |
| `SYMENC_SANITIZE` | OFF | Enable supported sanitizers. |
| `SYMENC_BUILD_TESTS` | ON | Build automated tests. |

The default build type is `RelWithDebInfo`.

## Command-Line Usage

After building, the executable is located at:

```text
build/src/symenc
```

### Display Version

```bash
./build/src/symenc --version
```

### Display Help

```bash
./build/src/symenc --help
```

### Encode a File

```bash
./build/src/symenc encode input.txt output.syenc
```

### Decode a File

```bash
./build/src/symenc decode output.syenc recovered.txt
```

### Verify the Result

```bash
cmp input.txt recovered.txt
```

No output from `cmp` indicates that both files are identical.

### Standard Input and Output

The CLI supports `-` as a standard input or output path:

```bash
printf 'HELLO' | ./build/src/symenc encode - -
```

Identical input and output file paths are refused.

File output uses a temporary file followed by a rename operation to avoid exposing incomplete protocol output on validation failure.

## Testing and Validation

The project includes automated tests covering:

- Protocol specification vectors.
- All 128 supported ASCII values.
- Encoder and decoder correctness.
- Tokenizer behavior.
- Error codes and byte offsets.
- Malformed and truncated inputs.
- Randomized round-trip testing.
- Differential decoder testing.
- Binary file handling.
- Command-line behavior.
- End-to-end encoding and decoding.

The test suite contains 126 assertions across 12 CTest test executables, plus process-level end-to-end coverage.

The implementation has been tested with GCC and Clang on Linux, including AddressSanitizer and UndefinedBehaviorSanitizer.

Cross-platform CI jobs for macOS and Windows are informational until their compatibility has been fully verified.

## Limitations and Security Considerations

SYENC is an encoding protocol, not a security mechanism.

- It provides no encryption or confidentiality.
- It uses no secret key.
- It provides no cryptographic integrity verification.
- It does not compress data.
- The current specification supports only 7-bit ASCII bytes.
- Encoded output is substantially larger than the original input.

Do not use SYENC as a replacement for cryptographic encryption, authentication, or secure data transport.

## Potential Applications

SYENC may be useful for experimentation and educational scenarios such as:

- Custom CTF encoding challenges.
- Cybersecurity and computer science education.
- Symbolic data representation.
- Parser and decoder testing.
- Protocol implementation exercises.
- Exploring reversible encoding systems.

These are potential use cases, not claims of production deployment.

## Specification and Versioning

The protocol specification is maintained separately from its implementation:

[`docs/SPEC.md`](docs/SPEC.md)

Current release:

**SYENC v0.1.1 — Frozen Specification**

Implementations should follow the specification rather than relying on language-specific behavior.

Future implementations in other programming languages should produce identical encoded byte sequences and decode the same valid inputs.

## Contributing

Contributions, technical reviews, bug reports, and suggestions are welcome.

Before proposing protocol changes, please review the frozen specification.

Implementation improvements and additional compatible implementations can be developed without changing the protocol itself.

## License

SYENC is distributed under the MIT License.

See [`LICENSE`](LICENSE) for details.

---

<p align="center">
  <strong>SYENC — One protocol. Three tokens. Reversible symbolic representation.</strong>
</p>
