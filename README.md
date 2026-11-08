# SYENC — Symbolic Encoding Protocol

[![CI](https://github.com/elkhaoudari/SYENC/actions/workflows/ci.yml/badge.svg)](https://github.com/elkhaoudari/SYENC/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)

**SYENC (Symbolic Encoding Protocol)** is a deterministic, reversible, symbolic encoding scheme that represents ASCII byte values using a small set of weighted tokens.

This repository contains the C++17 reference implementation of **SYENC Protocol v0.1.1**.

The protocol is designed around a simple mathematical model: each supported input byte is represented as a sequence of symbolic tokens whose values can be used to reconstruct the original byte.

> SYENC is an experimental symbolic encoding protocol. It is not encryption, compression, or a replacement for established binary-to-text encodings.

---

## Table of Contents

- [Overview](#overview)
- [Protocol Tokens](#protocol-tokens)
- [Encoding Model](#encoding-model)
- [Supported Range](#supported-range)
- [Reversibility](#reversibility)
- [Storage Characteristics](#storage-characteristics)
- [Protocol Rules](#protocol-rules)
- [Error Handling](#error-handling)
- [Implementation](#implementation)
- [Requirements](#requirements)
- [Build](#build)
- [Testing](#testing)
- [Command-Line Interface](#command-line-interface)
- [Project Structure](#project-structure)
- [Limitations](#limitations)
- [Protocol Specification](#protocol-specification)
- [License](#license)

---

## Overview

SYENC uses three symbolic tokens:

| Token | Value | Purpose |
|:---:|---:|---|
| `JIG` | +50 | Weighted value token |
| `€` | +1 | Unit value token |
| `SAW` | Delimiter | Terminates an encoded byte group |

Each input byte is converted into a symbolic group consisting of:

1. Zero, one, or two `JIG` tokens.
2. Zero or more `€` tokens.
3. One `SAW` delimiter.

The decoder reads each group, calculates its numerical value, and reconstructs the original byte.

### Example

Consider the ASCII character `l`.

Its decimal ASCII value is:

```text
108
```

The encoding parameters are:

$$
j = \left\lfloor \frac{108}{50} \right\rfloor = 2
$$

$$
e = 108 \bmod 50 = 8
$$

Therefore, its symbolic representation is:

```text
JIGJIG€€€€€€€€SAW
```

The decoder reconstructs the original value:

$$
N = 50j + e
$$

$$
N = 50(2) + 8 = 108
$$

---

## Protocol Tokens

SYENC v0.1.1 defines the following exact token representations:

| Token | UTF-8 / ASCII bytes | Meaning |
|---|---|---|
| `JIG` | `4A 49 47` | Adds 50 |
| `€` | `E2 82 AC` | Adds 1 |
| `SAW` | `53 41 57` | Terminates a group |

The decoder operates on raw input bytes and matches the exact token byte sequences.

It does not perform Unicode normalization or general Unicode character interpretation.

This ensures deterministic token recognition.

---

## Encoding Model

For every supported input byte $N$, the encoder calculates two values.

### Weighted component

$$
j = \left\lfloor \frac{N}{50} \right\rfloor
$$

### Unit component

$$
e = N \bmod 50
$$

Where:

- $N$ is the original byte value.
- $j$ is the number of `JIG` tokens.
- $e$ is the number of `€` tokens.

The resulting symbolic group is:

$$
E(N) = JIG^j \cdot EURO^e \cdot SAW
$$

Here, `EURO` represents the `€` token, and the exponents represent token repetition counts.

The decoder reconstructs the original value using:

$$
D(G) = 50j + e
$$

---

## Supported Range

SYENC v0.1.1 supports ASCII byte values:

$$
N \in \{0,1,\ldots,127\}
$$

The encoding parameters must satisfy the following constraints:

$$
j \in \{0,1,2\}
$$

$$
0 \leq e \leq 49
$$

Additionally:

$$
j = 2 \implies e \leq 27
$$

These constraints ensure that every valid encoded group represents exactly one supported ASCII byte.

---

## Reversibility

The fundamental correctness property of SYENC is reversibility.

For every supported input byte:

$$
D(E(N)) = N
$$

For a sequence of supported bytes, decoding the encoded sequence must reproduce the original input byte-for-byte.

The encoder and decoder are deterministic:

- The same input always produces the same encoded output.
- Every valid encoded group represents one unique supported byte.
- No secret key or external state is required.

---

## Storage Characteristics

SYENC is a symbolic representation, not a compression algorithm.

Each token occupies exactly 3 bytes:

- `JIG`: 3 bytes.
- `€`: 3 bytes.
- `SAW`: 3 bytes.

The encoded size depends on the original byte value.

### Minimum group size

For $N = 0$:

$$
j = 0,\qquad e = 0
$$

The encoded group contains only `SAW`.

Therefore:

$$
S_{\min} = 3 \text{ bytes}
$$

### Maximum group size

The maximum occurs at $N = 99$:

$$
j = 1,\qquad e = 49
$$

The group contains:

- 1 `JIG` token.
- 49 `€` tokens.
- 1 `SAW` token.

Therefore:

$$
S_{\max} = (1 + 49 + 1)\times 3
$$

$$
S_{\max} = 153 \text{ bytes}
$$

### Average encoded size

Across all 128 supported ASCII values, the average encoded size is:

$$
S_{\text{avg}} =
\frac{1}{128}
\sum_{N=0}^{127}
3\left(
\left\lfloor \frac{N}{50} \right\rfloor
+
(N \bmod 50)
+
1
\right)
$$

$$
S_{\text{avg}} = 71.765625
\text{ bytes per input byte}
$$

This means SYENC produces a significantly larger representation than the original input.

Its purpose is symbolic representation and protocol experimentation, not storage efficiency.

---

## Protocol Rules

The decoder enforces strict structural and numerical rules.

### Group structure

Every encoded byte group must follow this order:

```text
JIG* EURO* SAW
```

Where:

- `JIG*` represents zero to two `JIG` tokens.
- `EURO*` represents zero to forty-nine `€` tokens.
- `SAW` terminates the group.

The additional range restriction for two `JIG` tokens must also be respected.

### Token ordering

Tokens must appear in the correct order.

For example:

```text
JIG€SAW
```

is structurally valid.

However:

```text
€JIGSAW
```

is invalid because `JIG` cannot appear after a unit token.

### Whitespace

The decoder accepts the following whitespace characters between tokens:

- Space
- Tab
- Line Feed (`LF`)
- Carriage Return (`CR`)

Whitespace is not part of the encoded token representation.

The encoder emits no whitespace.

### Empty input

An empty input is valid and decodes to an empty output.

### Exact token matching

The decoder recognizes only the exact byte sequences defined by the protocol.

Unknown or malformed token sequences are rejected.

---

## Error Handling

SYENC v0.1.1 defines explicit protocol error codes.

| Error Code | Description |
|---|---|
| `E_NONASCII_INPUT` | Encoder received an unsupported input byte. |
| `E_BAD_TOKEN` | Unknown or malformed token encountered. |
| `E_ORDER` | Tokens appear in an invalid order. |
| `E_NONCANONICAL` | Token group does not follow the canonical representation rules. |
| `E_RANGE` | Token counts or reconstructed value exceed permitted limits. |
| `E_TRUNCATED` | Input ends before a complete encoded group is received. |

### Error offsets

Error offsets are zero-based byte offsets.

The decoder reports:

- The starting byte offset of a malformed token.
- The input length for `E_TRUNCATED`.

The decoder validates token structure and numerical constraints while processing the input.

When multiple violations occur within a token, the implementation applies its defined error precedence:

1. `E_ORDER`
2. `E_NONCANONICAL`
3. `E_RANGE`

Protocol errors must not result in partially returned decoded output.

For file output, the implementation uses temporary-file handling followed by a rename operation to avoid replacing the destination with incomplete output.

---

## Implementation

The reference implementation is written in C++17.

The project is organized into separate components for:

- Tokenization.
- Encoding.
- Decoding.
- Error handling.
- Command-line processing.
- Automated testing.

### Tokenizer

The tokenizer recognizes the protocol's exact token byte sequences.

Its interface includes:

```cpp
tokenizer.next_token(text, pos)
```

The tokenizer identifies the next valid token and advances through the input.

It does not interpret arbitrary Unicode characters or normalize token representations.

### Encoder

The encoder:

1. Receives input bytes.
2. Validates the supported input range.
3. Calculates $j$ and $e$.
4. Emits the corresponding symbolic tokens.
5. Terminates each group with `SAW`.

### Decoder

The decoder:

1. Reads the encoded byte stream.
2. Identifies tokens.
3. Validates token ordering.
4. Checks token counts and numerical constraints.
5. Reconstructs the original byte values.

The decoder rejects malformed protocol input instead of silently accepting invalid representations.

---

## Requirements

- C++17-compatible compiler.
- CMake.
- Standard C++ library.

No third-party runtime dependencies are required.

---

## Build

Clone the repository:

```bash
git clone https://github.com/elkhaoudari/SYENC.git
cd SYENC
```

Configure the project:

```bash
cmake -S . -B build
```

Build:

```bash
cmake --build build
```

Run the test suite:

```bash
ctest --test-dir build --output-on-failure
```

### Sanitizer build

To build with the project's sanitizer option:

```bash
cmake -S . -B build-san -DSYMENC_SANITIZE=ON
cmake --build build-san
ctest --test-dir build-san --output-on-failure
```

### Build options

| Option | Default | Description |
|---|---|---|
| `SYMENC_WERROR` | `ON` | Treat compiler warnings as errors. |
| `SYMENC_SANITIZE` | `OFF` | Enable supported sanitizers. |
| `SYMENC_BUILD_TESTS` | `ON` | Build the test suite. |

The default build configuration is `RelWithDebInfo`.

---

## Testing

The project includes automated tests covering the protocol implementation.

The test suite contains 126 assertions organized into 12 CTest tests.

Coverage includes:

- All 128 supported ASCII byte values.
- Encoder correctness.
- Decoder correctness.
- Round-trip validation.
- Seeded randomized round-trip tests.
- Differential testing against a second decoder.
- Invalid token handling.
- Token ordering violations.
- Noncanonical representations.
- Range violations.
- Truncated input.
- Binary input and output handling.
- CLI behavior and exit codes.
- File output handling.

Example manual round-trip test:

```bash
printf 'CTF{SYENC_IS_WORKING}' > message.txt

./build/src/symenc encode message.txt message.syenc

./build/src/symenc decode message.syenc recovered.txt

cmp -s message.txt recovered.txt \
    && echo "SUCCESS: SYENC WORKS!"
```

---

## Command-Line Interface

The compiled executable provides two main operations.

### Encode

```bash
./build/src/symenc encode INPUT OUTPUT
```

Example:

```bash
./build/src/symenc encode message.txt message.syenc
```

### Decode

```bash
./build/src/symenc decode INPUT OUTPUT
```

Example:

```bash
./build/src/symenc decode message.syenc recovered.txt
```

### Standard input and output

The special argument `-` represents standard input or standard output.

Examples:

```bash
cat message.txt | ./build/src/symenc encode - -
```

```bash
cat message.syenc | ./build/src/symenc decode - -
```

### Help and version

```bash
./build/src/symenc --help
```

```bash
./build/src/symenc --version
```

### Exit codes

| Exit Code | Meaning |
|---:|---|
| `0` | Successful execution |
| `1` | Protocol error |
| `2` | Invalid command-line usage |
| `3` | Input/output error |

The CLI refuses identical input and output file paths.

File output is written using temporary-file handling before the final rename operation.

---

## Project Structure

```text
symenc/
├── .github/
│   └── workflows/
│       └── ci.yml
├── docs/
│   └── SPEC.md
├── include/
│   └── symenc/
├── src/
├── tests/
├── .gitattributes
├── .gitignore
├── CMakeLists.txt
├── LICENSE
└── README.md
```

The `include/` directory contains public headers.

The `src/` directory contains the implementation.

The `tests/` directory contains automated tests and supporting test data.

The `docs/` directory contains the formal protocol specification.

---

## Limitations

SYENC v0.1.1 has a deliberately limited scope.

- Supports ASCII byte values from 0 to 127 only.
- Does not support arbitrary binary data as input.
- Does not provide encryption.
- Does not provide compression.
- Does not provide authentication or integrity verification.
- Does not use secret keys.
- Does not include checksums or error correction.
- Produces a larger output than the original input.

SYENC should not be used to protect confidential information or to verify data authenticity.

It is an experimental symbolic encoding protocol intended for learning, implementation experimentation, and protocol design exploration.

---

## Protocol Specification

The formal specification for SYENC v0.1.1 is available here:

[`docs/SPEC.md`](docs/SPEC.md)

The specification defines the protocol's token representation, encoding model, decoding rules, validation constraints, and error behavior.

The specification is the authoritative reference for protocol behavior.

---

## License

SYENC is released under the **MIT License**.

See the [`LICENSE`](LICENSE) file for the complete license text.

---

**SYENC — Symbolic Encoding Protocol v0.1.1**

A symbolic approach to representing and reconstructing ASCII byte values.
