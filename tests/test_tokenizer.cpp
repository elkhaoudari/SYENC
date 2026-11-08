#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "error_printers.hpp"
#include "symenc/protocol.hpp"
#include "symenc/tokenizer.hpp"
#include "test_framework.hpp"
#include "test_helpers.hpp"
#include "token_printers.hpp"

using symenc::Error;
using symenc::ErrorCode;
using symenc::Lex;
using symenc::next_token;
using symenc::Token;
using symenc::TokenKind;
using Status = symenc::Lex::Status;
using symtest::kEuroText;
using symtest::kJigText;
using symtest::kSawText;
using symtest::repeat;
using symtest::text_of;

namespace {

constexpr TokenKind J = TokenKind::Jig;
constexpr TokenKind S = TokenKind::Saw;
constexpr TokenKind E = TokenKind::Euro;

using Expected = std::vector<std::pair<TokenKind, std::size_t>>;

struct Run {
  std::vector<Token> tokens;
  Lex last{};
  std::size_t pos = 0;
};

Run lex_all(std::string_view text) {
  Run run;
  for (;;) {
    const Lex lex = next_token(text, run.pos);
    if (lex.status != Status::Token) {
      run.last = lex;
      return run;
    }
    run.tokens.push_back(lex.token);
  }
}

void expect_tokens(std::string_view text, const Expected& expected) {
  const Run run = lex_all(text);
  CHECK_EQ(run.tokens.size(), expected.size());
  for (std::size_t i = 0; i < run.tokens.size() && i < expected.size(); ++i) {
    CHECK_EQ(run.tokens[i].kind, expected[i].first);
    CHECK_EQ(run.tokens[i].offset, expected[i].second);
  }
  CHECK_EQ(run.last.status, Status::End);
  CHECK_EQ(run.pos, text.size());
}

void expect_failure(std::string_view text, std::size_t tokens_before, ErrorCode code,
                    std::size_t offset) {
  const Run run = lex_all(text);
  CHECK_EQ(run.tokens.size(), tokens_before);
  CHECK_EQ(run.last.status, Status::Failed);
  CHECK_EQ(run.last.error, Error({code, offset}));
}

}

SYMENC_TEST(each_token_is_recognized_and_consumes_three_bytes) {
  for (const auto& [text, kind] : std::vector<std::pair<std::string, TokenKind>>{
           {"JIG", J}, {"SAW", S}, {std::string(kEuroText), E}}) {
    std::size_t pos = 0;
    const Lex lex = next_token(text, pos);
    CHECK_EQ(lex.status, Status::Token);
    CHECK_EQ(lex.token.kind, kind);
    CHECK_EQ(lex.token.offset, std::size_t{0});
    CHECK_EQ(pos, std::size_t{3});
  }
}

SYMENC_TEST(JIGSAW_is_JIG_then_SAW) {
  expect_tokens("JIGSAW", {{J, 0}, {S, 3}});
}

SYMENC_TEST(adjacent_identical_tokens_split_cleanly) {
  expect_tokens("JIGJIG", {{J, 0}, {J, 3}});
  expect_tokens("SAWSAW", {{S, 0}, {S, 3}});
  expect_tokens(repeat(kEuroText, 3), {{E, 0}, {E, 3}, {E, 6}});
}

SYMENC_TEST(euro_tokens_are_three_bytes_wide_in_offsets) {
  const std::string text = "JIG" + std::string(kEuroText) + "SAW";
  expect_tokens(text, {{J, 0}, {E, 3}, {S, 6}});
}

SYMENC_TEST(tokenizer_knows_no_grammar) {
  expect_tokens(std::string(kEuroText) + "JIG", {{E, 0}, {J, 3}});
  expect_tokens("JIGJIGJIG", {{J, 0}, {J, 3}, {J, 6}});
}

SYMENC_TEST(whitespace_between_leading_and_trailing_is_skipped_but_counted) {
  const std::string text = " \t\r\nJIG  SAW\n";
  expect_tokens(text, {{J, 4}, {S, 9}});
}

SYMENC_TEST(empty_and_whitespace_only_input_just_ends) {
  expect_tokens("", {});
  expect_tokens("   \t\r\n  ", {});
}

SYMENC_TEST(only_the_four_whitespace_bytes_are_whitespace) {
  for (unsigned byte = 0; byte <= 255; ++byte) {
    const std::string text = text_of({byte});
    const bool is_ws = byte == 0x20 || byte == 0x09 || byte == 0x0A || byte == 0x0D;
    const Run run = lex_all(text);
    if (is_ws) {
      CHECK_EQ(run.last.status, Status::End);
    } else {
      CHECK_EQ(run.last.status, Status::Failed);
    }
  }
}

SYMENC_TEST(near_whitespace_bytes_are_rejected) {
  expect_failure(text_of({0x0B}), 0, ErrorCode::BadToken, 0);
  expect_failure(text_of({0x0C}), 0, ErrorCode::BadToken, 0);
  expect_failure(text_of({0xC2, 0xA0}), 0, ErrorCode::BadToken, 0);
  expect_failure(text_of({0x00}), 0, ErrorCode::BadToken, 0);
  expect_failure("SAW" + text_of({0xC2, 0xA0}), 1, ErrorCode::BadToken, 3);
}

SYMENC_TEST(whitespace_inside_a_token_is_bad_token_at_the_token_start) {
  expect_failure("J IG", 0, ErrorCode::BadToken, 0);
  expect_failure("JI G", 0, ErrorCode::BadToken, 0);
  expect_failure("JI ", 0, ErrorCode::BadToken, 0);
  expect_failure("SA\nW", 0, ErrorCode::BadToken, 0);
  expect_failure(text_of({0xE2, 0x20, 0x82, 0xAC}), 0, ErrorCode::BadToken, 0);
}

SYMENC_TEST(unknown_bytes_fail_at_their_own_offset) {
  expect_failure("X", 0, ErrorCode::BadToken, 0);
  expect_failure(" X", 0, ErrorCode::BadToken, 1);
  expect_failure("\n X", 0, ErrorCode::BadToken, 2);
  expect_failure("JIGX", 1, ErrorCode::BadToken, 3);
  expect_failure("JIGXSAW", 1, ErrorCode::BadToken, 3);
  expect_failure(std::string(kEuroText) + "X", 1, ErrorCode::BadToken, 3);
}

SYMENC_TEST(tokens_are_case_sensitive) {
  expect_failure("jig", 0, ErrorCode::BadToken, 0);
  expect_failure("Jig", 0, ErrorCode::BadToken, 0);
  expect_failure("saw", 0, ErrorCode::BadToken, 0);
  expect_failure("JIg", 0, ErrorCode::BadToken, 0);
}

SYMENC_TEST(a_mismatch_inside_a_token_reports_the_token_start) {
  expect_failure("JIX", 0, ErrorCode::BadToken, 0);
  expect_failure("SAW JX", 1, ErrorCode::BadToken, 4);
  expect_failure("SAX", 0, ErrorCode::BadToken, 0);
}

SYMENC_TEST(invalid_and_foreign_utf8_is_just_an_unknown_token) {
  const std::vector<std::string> inputs = {
      text_of({0x80}),
      text_of({0xFF}),
      text_of({0xC0, 0xAF}),
      text_of({0xED, 0xA0, 0x80}),
      text_of({0xF0, 0x9F, 0x98, 0x80}),
      text_of({0xC3, 0xA9}),
      text_of({0xEF, 0xBB, 0xBF}),
  };
  for (const std::string& text : inputs) expect_failure(text, 0, ErrorCode::BadToken, 0);
}

SYMENC_TEST(near_miss_euro_sequences_are_bad_token_at_the_start) {
  expect_failure(text_of({0xE2, 0x28, 0xA1}), 0, ErrorCode::BadToken, 0);
  expect_failure(text_of({0xE2, 0x82, 0xAD}) + "SAW", 0, ErrorCode::BadToken, 0);
  expect_failure(text_of({0xE2, 0x82, 0x28}), 0, ErrorCode::BadToken, 0);
  expect_failure(text_of({0xE2, 0x28}), 0, ErrorCode::BadToken, 0);
}

SYMENC_TEST(a_proper_token_prefix_at_end_of_input_is_truncated) {
  expect_failure("J", 0, ErrorCode::Truncated, 1);
  expect_failure("JI", 0, ErrorCode::Truncated, 2);
  expect_failure("S", 0, ErrorCode::Truncated, 1);
  expect_failure("SA", 0, ErrorCode::Truncated, 2);
  expect_failure(text_of({0xE2}), 0, ErrorCode::Truncated, 1);
  expect_failure(text_of({0xE2, 0x82}), 0, ErrorCode::Truncated, 2);
}

SYMENC_TEST(truncation_reports_the_input_length_not_the_token_start) {
  expect_failure("SAWS", 1, ErrorCode::Truncated, 4);
  expect_failure("SAW  J", 1, ErrorCode::Truncated, 6);
  expect_failure(std::string(kEuroText) + text_of({0xE2, 0x82}), 1, ErrorCode::Truncated, 5);
}

SYMENC_TEST(pos_is_unchanged_on_failure) {
  const std::string text = "JIGX";
  std::size_t pos = 0;
  CHECK_EQ(next_token(text, pos).status, Status::Token);
  CHECK_EQ(pos, std::size_t{3});
  CHECK_EQ(next_token(text, pos).status, Status::Failed);
  CHECK_EQ(pos, std::size_t{3});
}

SYMENC_TEST(lexing_can_resume_from_any_offset) {
  const std::string text = "JIGSAW";
  std::size_t pos = 3;
  const Lex lex = next_token(text, pos);
  CHECK_EQ(lex.status, Status::Token);
  CHECK_EQ(lex.token.kind, S);
  CHECK_EQ(lex.token.offset, std::size_t{3});
  CHECK_EQ(pos, std::size_t{6});
}

SYMENC_TEST(pos_beyond_the_end_is_treated_as_the_end) {
  const std::string text = "JIG";
  std::size_t pos = 99;
  CHECK_EQ(next_token(text, pos).status, Status::End);
  CHECK_EQ(pos, std::size_t{3});
}

SYMENC_TEST(every_single_byte_input_is_classified_correctly) {
  for (unsigned byte = 0; byte <= 255; ++byte) {
    const std::string text = text_of({byte});
    const Run run = lex_all(text);
    CHECK_EQ(run.tokens.size(), std::size_t{0});
    if (byte == 0x20 || byte == 0x09 || byte == 0x0A || byte == 0x0D) {
      CHECK_EQ(run.last.status, Status::End);
    } else if (byte == 0x4A || byte == 0x53 || byte == 0xE2) {
      CHECK_EQ(run.last.status, Status::Failed);
      CHECK_EQ(run.last.error, Error({ErrorCode::Truncated, 1}));
    } else {
      CHECK_EQ(run.last.status, Status::Failed);
      CHECK_EQ(run.last.error, Error({ErrorCode::BadToken, 0}));
    }
  }
}

SYMENC_TEST(every_second_byte_after_a_token_start_is_classified_correctly) {
  const unsigned firsts[3] = {0x4A, 0x53, 0xE2};
  const unsigned seconds[3] = {0x49, 0x41, 0x82};
  for (std::size_t t = 0; t < 3; ++t) {
    for (unsigned second = 0; second <= 255; ++second) {
      const std::string text = text_of({firsts[t], second});
      const Run run = lex_all(text);
      CHECK_EQ(run.last.status, Status::Failed);
      if (second == seconds[t]) {
        CHECK_EQ(run.last.error, Error({ErrorCode::Truncated, 2}));
      } else {
        CHECK_EQ(run.last.error, Error({ErrorCode::BadToken, 0}));
      }
    }
  }
}

SYMENC_TEST(every_third_byte_completes_or_breaks_a_token) {
  const unsigned prefix[3][2] = {{0x4A, 0x49}, {0x53, 0x41}, {0xE2, 0x82}};
  const unsigned thirds[3] = {0x47, 0x57, 0xAC};
  const TokenKind kinds[3] = {J, S, E};
  for (std::size_t t = 0; t < 3; ++t) {
    for (unsigned third = 0; third <= 255; ++third) {
      const std::string text = text_of({prefix[t][0], prefix[t][1], third});
      const Run run = lex_all(text);
      if (third == thirds[t]) {
        CHECK_EQ(run.tokens.size(), std::size_t{1});
        CHECK_EQ(run.tokens[0].kind, kinds[t]);
        CHECK_EQ(run.last.status, Status::End);
      } else {
        CHECK_EQ(run.tokens.size(), std::size_t{0});
        CHECK_EQ(run.last.error, Error({ErrorCode::BadToken, 0}));
      }
    }
  }
}

SYMENC_TEST(a_megabyte_of_tokens_lexes_with_exact_offsets) {
  const std::size_t count = 349525;
  const std::string text = repeat(kEuroText, count);
  const Run run = lex_all(text);
  CHECK_EQ(run.tokens.size(), count);
  bool offsets_ok = true;
  for (std::size_t i = 0; i < run.tokens.size(); ++i) {
    offsets_ok = offsets_ok && run.tokens[i].offset == 3 * i && run.tokens[i].kind == E;
  }
  CHECK(offsets_ok);
  CHECK_EQ(run.last.status, Status::End);
}

SYMENC_TEST(token_names_are_readable) {
  CHECK_EQ(symenc::name(J), std::string_view("JIG"));
  CHECK_EQ(symenc::name(S), std::string_view("SAW"));
  CHECK_EQ(symenc::name(E), std::string_view("EURO"));
}

SYMENC_TEST(library_token_bytes_match_independent_spellings) {
  CHECK_EQ(symtest::hex(kJigText), std::string("4A 49 47"));
  CHECK_EQ(symtest::hex(kSawText), std::string("53 41 57"));
  CHECK_EQ(symtest::hex(kEuroText), std::string("E2 82 AC"));
  CHECK_EQ(symenc::protocol::kJig[0], std::uint8_t{0x4A});
  CHECK_EQ(symenc::protocol::kSaw[2], std::uint8_t{0x57});
  CHECK_EQ(symenc::protocol::kEuro[1], std::uint8_t{0x82});
}
