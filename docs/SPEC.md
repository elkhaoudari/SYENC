# Symbolic Encoding Protocol — Version 0.1.1

*Status: frozen for implementation · Date: 2026-11-08 · Replaces: version 0.1 (judgment calls D1–D7 recorded in section 14)*

The key words MUST, MUST NOT, SHOULD and MAY are used as in RFC 2119.

## 1. Scope and terminology

**1.1 Scope.** This protocol defines a reversible, deterministic transformation between a sequence of ASCII bytes (0x00–0x7F) and a text built from exactly three tokens. Two independently written implementations MUST agree on every output, every error code and every error offset defined here.

**1.2 Non-goals.** v0.1 provides no encryption, no compression, no checksum or integrity check, no END marker, no header or version marker, and no support for bytes above 0x7F. It is a plain encoding with no confidentiality, and its output is much larger than its input (section 14).

**1.3 Terms.**

- *Byte*: an octet, 0x00–0xFF. All offsets and counts are in bytes.
- *Source bytes*: the encoder input and the decoder output.
- *Encoded text*: the encoder output and the decoder input.
- *Token*: one of JIG, SAW, EURO (section 2).
- *Group*: the tokens that encode one source byte: JIG tokens, then EURO tokens, then SAW.
- *j, e*: the number of JIG and EURO tokens in the current group.
- *Whitespace*: SP 0x20, HT 0x09, LF 0x0A, CR 0x0D, and nothing else.

## 2. Token definitions

| Token | Byte sequence | Display | Meaning | Length |
| --- | --- | --- | --- | --- |
| JIG | 4A 49 47 | JIG | +50 | 3 bytes |
| SAW | 53 41 57 | SAW | ends the group | 3 bytes |
| EURO | E2 82 AC | € (U+20AC) | +1 | 3 bytes |

Matching is case-sensitive. EURO is matched as these exact three bytes and never as a decoded code point. All tokens are exactly 3 bytes and start with distinct bytes (0x4A, 0x53, 0xE2), so a group with t tokens occupies exactly 3t bytes.

## 3. Formal grammar

Notation: ABNF (RFC 5234, an EBNF dialect) with repetition counts n\*m.

```
encoded-text = *ws *( token *ws )          ; lexical layer
token        = JIG / SAW / EURO
JIG   = %x4A.49.47
SAW   = %x53.41.57
EURO  = %xE2.82.AC
ws    = %x20 / %x09 / %x0A / %x0D

; syntactic layer: the token sequence, whitespace removed
stream = *group
group  = ( 0*1JIG 0*49EURO / 2JIG 0*27EURO ) SAW
```

The group rule admits exactly 50 + 50 + 28 = 128 distinct groups, one per ASCII value. The grammar is therefore itself the canonical-form rule: every grammar-valid group is the canonical encoding of exactly one byte.

## 4. Mathematical encoding rules

For a byte value N with 0 ≤ N ≤ 127:

- Encode: j = ⌊N / 50⌋, e = N mod 50, group = JIG×j, then EURO×e, then SAW.
- Decode: N = 50·j + e.
- Domain: j ∈ {0, 1, 2}, e ∈ {0 … 49}, N ≤ 127 (so j = 2 implies e ≤ 27).
- Group length: 3·(j + e + 1) bytes.
- Correctness: the division algorithm gives a unique (j, e) with 0 ≤ e < 50, so decode(encode(N)) = N. Encode is injective and the grammar admits exactly 128 groups, so encode is a bijection from 0..127 onto the valid groups.

The encoded text of a byte string is the concatenation of its groups in order. The empty string encodes to the empty text.

## 5. Canonical representation rules

- **C1.** Encoder output is groups only: no whitespace, no newline (not even a trailing one), no BOM, no header, no trailer.
- **C2.** The encoder MUST be deterministic: equal input gives byte-identical output.
- **C3.** The decoder accepts only grammar-valid groups, so every accepted group is canonical. No value has a second spelling (50 is JIG, never 50 EURO).
- **C4.** The decoder tolerates whitespace between tokens, which is not canonical. For every source s, decode(encode(s)) = s. For every text t the decoder accepts, encode(decode(t)) equals t with all whitespace removed.
- **C5.** Token order inside a group is fixed: JIG, then EURO, then SAW.

## 6. Tokenization rules

- **T1.** The tokenizer works on raw bytes. It MUST NOT decode UTF-8 in general, normalize Unicode, or consult the locale. A valid UTF-8 character other than € is simply an unknown token.
- **T2.** To read the next token at position p:
  1. Skip whitespace bytes. If the end of input is reached, there are no more tokens.
  2. Let start = p. Select the token by the first byte: 0x4A is JIG, 0x53 is SAW, 0xE2 is EURO. Any other byte is E\_BAD\_TOKEN at start.
  3. Compare the remaining two bytes in order. If the input ends first, the error is E\_TRUNCATED. If a byte differs (whitespace included), the error is E\_BAD\_TOKEN at start.
  4. Otherwise emit the token and continue at start + 3.
- **T3. Determinism.** First bytes are distinct and all tokens have fixed length 3, so there is exactly one tokenization. No backtracking and no lookahead beyond the current token are needed.
- **T4. Examples.** JIGSAW is JIG, SAW. JIGJIG is JIG, JIG. SAWSAW is SAW, SAW. The text J, I, SP, G is E\_BAD\_TOKEN, because whitespace is never allowed inside a token.
- **T5. Valid versus malformed.** A text is valid if and only if (a) tokenization consumes the whole input without a lexical error and (b) the token sequence is accepted by the state machine of section 7 and ends in state IDLE. Otherwise the first violation found by one left-to-right scan is reported (section 8).
- **T6. Prefix at end of input.** A proper prefix of a token at the end of input (E2, E2 82, J, JI, S, SA) is E\_TRUNCATED, because more bytes could have completed it. A prefix followed by a non-matching byte is E\_BAD\_TOKEN.
- **T7. Whitespace.** Whitespace is allowed only between tokens, and leading, trailing and repeated whitespace are all allowed. An empty input, or one containing only whitespace, decodes to empty output.

## 7. Decoder state machine

The tokenizer feeds tokens one at a time to this machine. Each token is tokenized first, then checked.

| State | Meaning | Invariant |
| --- | --- | --- |
| IDLE | between groups (initial; the only accepting state at end of input) | j = 0, e = 0 |
| JIGS | one or two JIG seen | 1 ≤ j ≤ 2, e = 0 |
| UNITS | at least one EURO seen | e ≥ 1 |

| State | JIG | EURO | SAW | End of input |
| --- | --- | --- | --- | --- |
| IDLE | j=1, go JIGS | e=1, go UNITS | emit 0, stay IDLE | accept |
| JIGS, j=1 | j=2, stay | e=1, go UNITS | emit 50, go IDLE | E\_TRUNCATED |
| JIGS, j=2 | E\_NONCANONICAL | e=1, go UNITS | emit 100, go IDLE | E\_TRUNCATED |
| UNITS | E\_ORDER | e+=1 after checks | emit 50j+e, go IDLE | E\_TRUNCATED |

**Checks on JIG, in order:** (1) e > 0 gives E\_ORDER; (2) j = 2 gives E\_NONCANONICAL.

**Checks on EURO, in order:** (1) e = 49 gives E\_NONCANONICAL; (2) 50j + e + 1 > 127 gives E\_RANGE.

**Principle (prefix viability).** An error is raised at the first token after which the current group can no longer be completed into any valid group. SAW therefore never fails. After the first EURO of a group, every JIG is rejected (E\_ORDER) until a SAW starts the next group.

### Decoder pseudocode

```
function decode(input):
  out = empty buffer; n = length(input); p = 0; j = 0; e = 0
  loop:
    while p < n and input[p] in {20, 09, 0A, 0D}: p += 1
    if p == n: break
    start = p
    case input[p]:
      4A: pattern = 4A 49 47; tok = JIG
      53: pattern = 53 41 57; tok = SAW
      E2: pattern = E2 82 AC; tok = EURO
      else: fail(E_BAD_TOKEN, start)
    for k in 1 .. 2:
      if start + k >= n: fail(E_TRUNCATED, n)
      if input[start + k] != pattern[k]: fail(E_BAD_TOKEN, start)
    p = start + 3
    if tok == JIG:
      if e > 0: fail(E_ORDER, start)
      if j == 2: fail(E_NONCANONICAL, start)
      j += 1
    elif tok == EURO:
      if e == 49: fail(E_NONCANONICAL, start)
      if 50*j + e + 1 > 127: fail(E_RANGE, start)
      e += 1
    else:
      append(out, 50*j + e); j = 0; e = 0
  if j > 0 or e > 0: fail(E_TRUNCATED, n)
  return out

function encode(source):
  for k in 0 .. length(source) - 1:
    if source[k] > 127: fail(E_NONASCII_INPUT, k)
  out = empty
  for b in source: append JIG x (b / 50), EURO x (b mod 50), SAW
  return out
```

`fail` aborts the whole operation and returns no output. Both functions run in O(n) time. The decoder needs O(1) memory beyond its output.

## 8. Error codes and precedence

| Code | Raised by | Condition | Offset |
| --- | --- | --- | --- |
| E\_NONASCII\_INPUT | encoder | a source byte above 0x7F | first such byte in the source |
| E\_BAD\_TOKEN | decoder | a byte that starts no token, or a token prefix followed by a non-matching byte | first byte of the failed token |
| E\_ORDER | decoder | a JIG after an EURO in the same group | that JIG |
| E\_NONCANONICAL | decoder | a third JIG, or a 50th EURO, in one group | that token |
| E\_RANGE | decoder | an EURO that would make 50j + e exceed 127 (j = 2, the 28th EURO) | that EURO |
| E\_TRUNCATED | decoder | input ends inside a token or inside a group | input length |

No new codes are needed. Invalid UTF-8 and truncated € map onto the existing codes because the tokenizer never decodes UTF-8: it only matches exact byte patterns.

**Precedence rules**

- **P1.** Only the first error of a single left-to-right scan is reported, and scanning stops.
- **P2.** Each token is checked lexically before it is checked by the state machine, and tokens are processed in order. So an earlier violation always wins over a later one.
- **P3.** Within one token the order is E\_ORDER, then E\_NONCANONICAL, then E\_RANGE. This is why a third JIG (value 150, also out of range) is E\_NONCANONICAL. Range is checked eagerly (decision D3): E\_RANGE is raised at the EURO that would push the value above 127, never deferred to SAW.
- **P4.** E\_TRUNCATED is considered only after every byte was consumed without another error.
- **P5.** E\_NONASCII\_INPUT belongs to the encoder only. The encoder reports the first offending byte and stops. The decoder never raises it, because decoded values are at most 127.

**Case analysis** (encoded text with no whitespace; offsets follow section 9). In the Input column, commas only separate tokens for readability and ×n means n repetitions. Literal ASCII text is written as is, with its hex in parentheses.

| # | Input | Result | Reason |
| --- | --- | --- | --- |
| 1 | EURO×50, SAW | E\_NONCANONICAL @147 | The 50th € (bytes 147–149) would make e = 50. The value 50 has the canonical form JIG SAW. Not a range problem. |
| 2 | JIG, JIG, JIG, SAW | E\_NONCANONICAL @6 | The third JIG gives j = 3. Value 150 is also above 127, but P3 puts E\_NONCANONICAL first. |
| 3 | EURO, JIG, SAW | E\_ORDER @3 | JIG after EURO in the same group. |
| 4 | JIG, JIG, EURO×28, SAW | E\_RANGE @87 | The first 27 € give 127, valid. The 28th (bytes 87–89) would give 128. |
| 5 | EURO, EURO | E\_TRUNCATED @6 | Tokens are valid but the group has no SAW. Offset is the input length. |
| 6 | JIGXSAW (hex 4A 49 47 58 53 41 57) | E\_BAD\_TOKEN @3 | X starts no token. |
| 7 | byte FF | E\_BAD\_TOKEN @0 | Invalid UTF-8 is just an unmatched byte. |
| 8 | E2 82 (end) | E\_TRUNCATED @2 | Proper prefix of EURO at end of input (T6). |
| 9 | E2 82 AD, SAW | E\_BAD\_TOKEN @0 | U+20AD is not €. The mismatch is reported at the token start. |
| 10 | E2 28 A1 | E\_BAD\_TOKEN @0 | Second byte differs from 82. |
| 11 | C3 A9, SAW (é) | E\_BAD\_TOKEN @0 | Valid UTF-8, but not a token. |
| 12 | JIG, JIG, EURO×27, JIG, SAW | E\_ORDER @87 | Value 127 is valid so far, so the JIG is the first violation. |
| 13 | SAWS (hex 53 41 57 53) | E\_TRUNCATED @4 | A complete SAW token, then S, a proper prefix of a token, at end of input (T6). |
| 14 | JIG | E\_TRUNCATED @3 | Group has no SAW. |
| 15 | JIX (hex 4A 49 58) | E\_BAD\_TOKEN @0 | Third byte mismatches. |

## 9. Byte offset semantics

- **O1.** Offsets are zero-based byte offsets. The first byte of the input is offset 0.
- **O2.** Decoder offsets index the encoded text exactly as read. They count every byte, including whitespace and any BOM. Encoder offsets index the source bytes. Offsets never count code points, tokens or groups.
- **O3.** Token errors report the first byte of the offending token (J, S or E2), never an interior byte.
- **O4.** E\_BAD\_TOKEN reports the first byte of the lexeme being matched, even if the mismatch is at its second or third byte. For an unknown byte it is that byte. For invalid UTF-8 it is the first byte that cannot start a token, or the E2 of a failed EURO.
- **O5.** E\_TRUNCATED reports the input length (one past the last byte), even when the input ends with ignored whitespace.
- **O6.** Whitespace consumes offsets but never causes an error.

| Input (hex) | Result |
| --- | --- |
| 0A 20 58 | E\_BAD\_TOKEN @2 (LF, SP, then X) |
| E2 82 AC 4A 49 47 | E\_ORDER @3 |
| E2 82 AC 20 20 | E\_TRUNCATED @5 |
| 53 41 57 C2 A0 | E\_BAD\_TOKEN @3 (NBSP is not whitespace) |

A RECOMMENDED message format is: `E_BAD_TOKEN at byte offset 3`.

## 10. Binary I/O requirements

- **B1.** Read the whole input in binary mode (std::ios::binary). No newline translation, no charset conversion, no BOM handling, no locale dependence. On Windows, stdin and stdout MUST be switched to binary mode.
- **B2.** Use unsigned byte types (std::uint8\_t or unsigned char) for all byte arithmetic and comparison, never plain char.
- **B3.** The encoder never normalizes or alters content. CR, LF and CRLF encode as the groups for 13, 10 and 13 followed by 10.
- **B4.** The decoder writes in binary mode. NUL and all control bytes are written verbatim, and no trailing newline is added.
- **B5.** Empty input gives empty output for both operations. A 0-byte output file MUST be created in that case.
- **B6. No partial output.** On any protocol failure (any of the six error codes), no output file may be created or modified and no byte may be written to stdout. Procedure: finish validation and the full conversion in a memory buffer first; only then open the destination. For a file destination, write to a temporary file in the same directory and rename it over the destination. On any failure, including a physical I/O failure, delete the temporary file and leave an existing destination untouched. For stdout, write only after the whole buffer succeeded. This guarantee covers protocol failures only: a physical I/O failure while writing to stdout (closed pipe, full disk) cannot guarantee that zero bytes were written, and the implementation MUST then exit with code 3.
- **B7.** Implementations SHOULD refuse identical input and output paths.

## 11. Test vectors

Notation: EURO×n means n consecutive € tokens. Tokens are separated by spaces for readability only; real text has no spaces unless a case says so.

### 11.1 Encoding vectors

| ID | Source (hex) | Encoded | Bytes |
| --- | --- | --- | --- |
| E01 | empty | empty | 0 |
| E02 | 00 (NUL) | SAW | 3 |
| E03 | 09 (TAB) | EURO×9 SAW | 30 |
| E04 | 0A (LF) | EURO×10 SAW | 33 |
| E05 | 0D 0A (CRLF) | EURO×13 SAW EURO×10 SAW | 75 |
| E06 | 20 (space) | EURO×32 SAW | 99 |
| E07 | 41 (A) | JIG EURO×15 SAW | 51 |
| E08 | 48 (H) | JIG EURO×22 SAW | 72 |
| E09 | 69 (i) | JIG JIG EURO×5 SAW | 24 |
| E10 | 6C (l) | JIG JIG EURO×8 SAW | 33 |
| E11 | 64 (d) | JIG JIG SAW | 9 |
| E12 | 7F (DEL) | JIG JIG EURO×27 SAW | 90 |
| E13 | 31 (N = 49) | EURO×49 SAW | 150 |
| E14 | 32 (N = 50) | JIG SAW | 6 |
| E15 | 63 (N = 99) | JIG EURO×49 SAW | 153 |
| E16 | 64 (N = 100) | JIG JIG SAW | 9 |

Byte-level references: N = 0 is `53 41 57`. N = 1 is `E2 82 AC 53 41 57`. N = 50 is `4A 49 47 53 41 57`. N = 100 is `4A 49 47 4A 49 47 53 41 57`. Decoding the text JIGSAW gives the single byte 0x32.

### 11.2 Round-trip suite

| ID | Input | Expected |
| --- | --- | --- |
| R1 | each single byte N = 0..127 (128 files) | exact group by the formula, length 3·(⌊N/50⌋ + N mod 50 + 1), decodes back to the same byte |
| R2 | bytes 0..127 ascending; R3 descending | identical round trip, encoded length 9186 bytes each |
| R4 | 00 31 32 63 64 7F (the boundaries 0, 49, 50, 99, 100, 127) | identical round trip, 137 tokens, 411 bytes |
| R5 | 1000 × 00 | round trip, encoded is 1000 × SAW, 3000 bytes |
| R6 | 0A; 0D 0A; 0D; 0A 0D; 61 0D 0A 62; 61 0A | exact preservation, all encodings pairwise distinct |
| R7 | seed 0xC0FFEE, 10000 cases, length uniform 0..4096, bytes uniform 0..127, plus five 1 MiB cases | identical round trip |
| R8 | encode(decode(encode(x))) for every case above | equals encode(x) |
| R9 | valid encoding with random runs of SP, HT, LF, CR inserted only between tokens | decodes to x |
| R10 | real ASCII sources in C, Python, JavaScript, PHP, Rust, Go, once with LF and once with CRLF | byte-identical (cmp) after the round trip |

### 11.3 Decoder error vectors

The 15 cases of section 8 are normative test vectors. Additional cases:

| ID | Input | Expected |
| --- | --- | --- |
| N1 | lowercase text jig then SAW | E\_BAD\_TOKEN @0 |
| N2 | JIG, C2 A0, SAW | E\_BAD\_TOKEN @3 |
| N3 | EF BB BF, SAW (BOM) | E\_BAD\_TOKEN @0 |
| N4 | 00 | E\_BAD\_TOKEN @0 |
| N5 | J, SP, I, G, SAW | E\_BAD\_TOKEN @0 |
| N6 | 80; C0 AF; ED A0 80; F0 9F 98 80 | E\_BAD\_TOKEN @0 each |
| N7 | 53 41 57 80 | E\_BAD\_TOKEN @3 |
| N8 | SAW then EURO | E\_TRUNCATED @6 |
| N9 | JIG, JIG, EURO×28, JIG, SAW | E\_RANGE @87 (earlier violation wins over E\_ORDER) |

### 11.4 Whitespace vectors (decoder must accept)

| ID | Input | Expected output |
| --- | --- | --- |
| W1 | JIG, CR LF, EURO, EURO, SP, SAW, LF | 34 (one byte, value 52) |
| W2 | three spaces only | empty output, success |
| W3 | SAW, TAB, SAW | 00 00 |
| W4 | LF, then JIGSAW | 32 |

### 11.5 Encoder error vectors

| ID | Source (hex) | Expected |
| --- | --- | --- |
| X1 | 80 | E\_NONASCII\_INPUT @0 |
| X2 | 41 42 80 | E\_NONASCII\_INPUT @2 |
| X3 | C3 A9 | E\_NONASCII\_INPUT @0 (first offending byte only) |
| X4 | 1048576 bytes of 61, then FF | E\_NONASCII\_INPUT @1048576 |
| X5 | EF BB BF, 78 | E\_NONASCII\_INPUT @0 (BOM is not stripped) |

### 11.6 Property and fuzz tests

- **F1 Truncation sweep.** For every valid encoding in R1–R7 and every prefix length k: if k falls on a group boundary the prefix decodes successfully; otherwise the result MUST be E\_TRUNCATED @k. No other code may appear.
- **F2 Random bytes.** Feed random byte strings to the decoder. It never crashes. On success, encode(output) equals the input with whitespace removed. On error, the code is one of the six defined and offset ≤ input length.
- **F3 Mutation sweep.** Change each single byte of valid encodings in turn. The result is either success or a defined error, never a crash. Run under ASan and UBSan.

### 11.7 CLI and atomicity

- **A1.** Destination file contains KEEP. Decode of an invalid input fails with exit code 1. Afterwards the destination still contains KEEP and no temporary file remains.
- **A2.** Destination does not exist. After a failed run it still does not exist.
- **A3.** Output to stdout with a protocol-invalid input writes 0 bytes to stdout (physical I/O failures are outside this test).
- **A4.** A successful run leaves the destination byte-identical to the expected output and no temporary file.

## 12. Acceptance criteria

1. Every vector in section 11 passes with exact bytes, error code and offset.
2. R1–R10 pass, including byte comparison of real source files.
3. F1–F3 pass under ASan and UBSan with no undefined behavior.
4. A1–A4 pass.
5. Decoding is a single pass in O(n) time with O(1) memory beyond the output.
6. Encoding is deterministic: two runs give byte-identical output.
7. The code builds without warnings at -Wall -Wextra -Wconversion with -std=c++17 on GCC and Clang.
8. Two independently written implementations produce identical results on all vectors.

## 13. C++17 implementation architecture

**Modules:** `error` (codes and result type), `tokenizer` (section 6), `codec` (section 7, no I/O), `io` (binary read and atomic write), `cli` (argument parsing, exit codes).

```cpp
// error.hpp
namespace symenc {
enum class ErrorCode { NonAsciiInput, BadToken, Order, NonCanonical, Range, Truncated };
struct Error { ErrorCode code; std::size_t offset; };
std::string_view name(ErrorCode) noexcept;   // E_BAD_TOKEN and so on
template <class T> using Result = std::variant<T, Error>;

// tokenizer.hpp
enum class TokenKind : std::uint8_t { Jig, Saw, Euro };
struct Token { TokenKind kind; std::size_t offset; };   // length is always 3
struct Lex { enum class Status { Token, End, Failed } status; Token token; Error error; };
Lex next_token(std::string_view text, std::size_t& pos) noexcept;

// codec.hpp
using Bytes = std::vector<std::uint8_t>;
Result<std::string> encode(const Bytes& source);
Result<Bytes> decode(std::string_view text);

// io.hpp
struct IoError : std::runtime_error { using std::runtime_error::runtime_error; };
Bytes read_binary(const std::filesystem::path& file);
Bytes read_binary(std::istream& in);
void write_binary_atomic(const std::filesystem::path& dest, std::string_view data);
void write_binary(std::ostream& out, std::string_view data);
}
```

**Error handling.** Protocol errors are returned values (Result), never exceptions. I/O failures throw IoError, and allocation failure propagates as std::bad\_alloc. The CLI catches both. Exit codes: 0 success, 1 protocol error (message on stderr), 2 usage error, 3 I/O error.

**Byte handling.** The tokenizer reads the text through `static_cast<unsigned char>` and never uses plain char arithmetic.

**No partial output.** The data flow is read all, then codec, then write. The writer is called only on success. For files, `write_binary_atomic` creates a temporary file next to the destination (opened binary), writes, flushes, closes, checks the stream state, then calls `std::filesystem::rename`. Any failure removes the temporary file with the error\_code overload. A physical I/O failure on stdout cannot be undone and is reported with exit code 3 (see B6). Because the codec finishes before any file is opened, a protocol error never touches the filesystem.

**CLI:** `symenc encode IN OUT` and `symenc decode IN OUT`, where `-` means stdin or stdout.

## 14. Known limitations and open points

### 14.1 Limitations

- **Size.** Every token is 3 bytes. Over printable ASCII (32–126) the average group is about 26 tokens (about 79 bytes). Over all 128 values it is about 24 tokens (72 bytes). The best case is 3 bytes (N = 0) and the worst case is 153 bytes (N = 99); N = 50 is 6 bytes. This format is not compressed.
- **No confidentiality.** The mapping is public and keyless. Anyone with this document decodes it instantly.
- **No integrity.** A text cut at a group boundary decodes without error, and corruption that keeps the grammar valid goes undetected.
- **ASCII only.** Bytes above 0x7F are rejected, not approximated.
- **Encoded text is not ASCII.** € is non-ASCII. Re-encoding the file to Latin-1 or similar destroys it, and an editor-added BOM is E\_BAD\_TOKEN @0.
- **Memory.** The encoder buffer can be up to 153 times the input. This is acceptable for v0.1 but impractical for very large files. A streaming variant has the same observable behavior and is a conformant alternative.
- **No self-identification.** There is no magic number or version. Later token additions (such as END) will make a v0.1 decoder fail with E\_BAD\_TOKEN, which is the intended forward behavior.

### 14.2 The seven judgment calls and their status

| ID | Question | Decision | Status | Alternative not taken |
| --- | --- | --- | --- | --- |
| D1 | Whitespace in decoder input | Accept SP, HT, LF, CR between tokens, including leading and trailing; many texts decode to the same bytes | Decided | Strict mode rejecting all whitespace |
| D2 | EOF in the middle of a token | E\_TRUNCATED | Decided | E\_BAD\_TOKEN |
| D3 | When counts and range are checked | Eagerly, at the first token that makes the group impossible; E\_RANGE is raised at once | Decided | Defer to the SAW token |
| D4 | Offset of a malformed token | Its first byte | Default, accepted | The first mismatching byte |
| D5 | E\_TRUNCATED offset | The input length | Default, accepted | Start of the incomplete token or group |
| D6 | Rename over an existing file on Windows | Use std::filesystem::rename; replacement semantics are platform-defined, so verify on the target platform | Default; platform check during implementation, no effect on the format | An explicit platform replace API |
| D7 | Empty file | Valid in both directions | Default, accepted | Reject empty input |

The offsets in cases 4, 12 and N9 follow from D3 (eager checking) and are normative.

### 14.3 Settled in 0.1.1

- EOF inside a group (valid tokens read, final SAW missing) is E\_TRUNCATED at the input length.
- Token matching is exact and byte-level. JIGSAW is JIG followed by SAW.
- Case 13 of section 8 is the literal text SAWS (hex 53 41 57 53): E\_TRUNCATED at offset 4.
- The no-partial-output rule (B6) covers protocol failures only. A physical I/O failure while writing to stdout cannot guarantee zero bytes written. Temporary-file output stays for file destinations.
- Every length, count and offset in sections 8, 9 and 11 was recomputed; the best-case group is 3 bytes (N = 0).
- The grammar, pseudocode, state machine and test vectors agree on j ≤ 2, e ≤ 49, N ≤ 127, the order of checks (E\_ORDER, E\_NONCANONICAL, E\_RANGE) and the end-of-input rule.

### 14.4 Freeze

D4, D5 and D7 are accepted defaults. D6 is a platform check, not a format question. Any change to this document requires a new version number.
