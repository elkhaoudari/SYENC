#include <cstddef>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "symenc/cli.hpp"
#include "symenc/codec.hpp"
#include "symenc/io.hpp"
#include "temp_dir.hpp"
#include "test_framework.hpp"
#include "test_helpers.hpp"

#ifndef SYMENC_TEST_DATA_DIR
#error "SYMENC_TEST_DATA_DIR must be defined by CMake"
#endif

namespace fs = std::filesystem;
using symenc::cli::kExitIo;
using symenc::cli::kExitOk;
using symenc::cli::kExitProtocolError;
using symenc::cli::kExitUsage;
using symtest::TempDir;

namespace {

struct Outcome {
  int code;
  std::string out;
  std::string err;
};

Outcome run(const std::vector<std::string>& args, const std::string& stdin_data = "") {
  std::istringstream in(stdin_data);
  std::ostringstream out;
  std::ostringstream err;
  const int code = symenc::cli::run(args, in, out, err);
  return {code, out.str(), err.str()};
}

void write_file(const fs::path& path, const std::string& data) {
  std::ofstream out(path, std::ios::binary);
  out.write(data.data(), static_cast<std::streamsize>(data.size()));
}

std::string read_file(const fs::path& path) {
  return std::string(symenc::as_text(symenc::read_binary(path)));
}

std::size_t entries_in(const fs::path& dir) {
  std::size_t count = 0;
  for (const auto& entry : fs::directory_iterator(dir)) {
    static_cast<void>(entry);
    ++count;
  }
  return count;
}

std::string with_crlf(const std::string& text) {
  std::string out;
  for (const char c : text) {
    if (c == '\n') out.push_back('\r');
    out.push_back(c);
  }
  return out;
}

std::string library_encode(const std::string& source) {
  const auto result = symenc::encode(symtest::bytes_of_text(source));
  return symenc::failed(result) ? std::string("<error>") : symenc::value_of(result);
}

}

SYMENC_TEST(version_and_help_exit_zero) {
  const Outcome v = run({"--version"});
  CHECK_EQ(v.code, kExitOk);
  CHECK_EQ(v.out, std::string("symenc (spec 0.1.1)\n"));
  CHECK_EQ(run({"--help"}).code, kExitOk);
  CHECK_EQ(run({"-h"}).code, kExitOk);
}

SYMENC_TEST(bad_command_lines_exit_two_with_usage_on_stderr) {
  const std::vector<std::vector<std::string>> bad = {
      {}, {"encode"}, {"encode", "a"}, {"encode", "a", "b", "c"}, {"frobnicate", "a", "b"},
      {"--version", "extra"}, {"ENCODE", "a", "b"}};
  for (const auto& args : bad) {
    const Outcome o = run(args);
    CHECK_EQ(o.code, kExitUsage);
    CHECK(o.err.find("usage:") != std::string::npos);
    CHECK_EQ(o.out, std::string());
  }
}

SYMENC_TEST(identical_input_and_output_paths_are_refused) {
  TempDir dir;
  write_file(dir.file("f"), "x");
  CHECK_EQ(run({"encode", dir.file("f").string(), dir.file("f").string()}).code, kExitUsage);
  const std::string alias = (dir.path() / "." / "f").string();
  CHECK_EQ(run({"encode", dir.file("f").string(), alias}).code, kExitUsage);
  CHECK_EQ(read_file(dir.file("f")), std::string("x"));
}

SYMENC_TEST(R10_real_source_files_round_trip_byte_for_byte_with_LF_and_CRLF) {
  TempDir dir;
  const std::vector<std::string> names = {"hello.c", "hello.py", "hello.js",
                                          "hello.php", "hello.rs", "hello.go"};
  for (const std::string& name : names) {
    const std::string lf = read_file(fs::path(SYMENC_TEST_DATA_DIR) / name);
    CHECK(!lf.empty());
    CHECK(lf.find('\r') == std::string::npos);
    const std::string crlf = with_crlf(lf);
    CHECK(crlf != lf);

    std::string encoded_lf;
    for (const std::string& original : {lf, crlf}) {
      write_file(dir.file("src"), original);
      const Outcome enc = run({"encode", dir.file("src").string(), dir.file("enc").string()});
      CHECK_EQ(enc.code, kExitOk);
      CHECK(read_file(dir.file("enc")) == library_encode(original));
      const Outcome dec = run({"decode", dir.file("enc").string(), dir.file("back").string()});
      CHECK_EQ(dec.code, kExitOk);
      CHECK(read_file(dir.file("back")) == original);
      if (original == lf) encoded_lf = read_file(dir.file("enc"));
      else CHECK(read_file(dir.file("enc")) != encoded_lf);
    }
  }
}

SYMENC_TEST(stdin_to_stdout_carries_NUL_and_control_bytes) {
  const std::string source = symtest::text_of({0x00, 0x09, 0x0A, 0x0D, 0x0A, 0x20, 0x7F, 0x00});
  const Outcome enc = run({"encode", "-", "-"}, source);
  CHECK_EQ(enc.code, kExitOk);
  CHECK(enc.out == library_encode(source));
  const Outcome dec = run({"decode", "-", "-"}, enc.out);
  CHECK_EQ(dec.code, kExitOk);
  CHECK(dec.out == source);
}

SYMENC_TEST(the_encoded_text_has_no_trailing_newline_and_decoding_tolerates_one) {
  const Outcome enc = run({"encode", "-", "-"}, "A");
  CHECK_EQ(enc.out.substr(enc.out.size() - 3), std::string("SAW"));
  CHECK_EQ(run({"decode", "-", "-"}, enc.out + "\n").out, std::string("A"));
}

SYMENC_TEST(D7_empty_input_is_valid_in_both_directions) {
  TempDir dir;
  write_file(dir.file("empty"), "");
  CHECK_EQ(run({"encode", dir.file("empty").string(), dir.file("e.out").string()}).code, kExitOk);
  CHECK(fs::exists(dir.file("e.out")));
  CHECK_EQ(fs::file_size(dir.file("e.out")), std::uintmax_t{0});
  CHECK_EQ(run({"decode", dir.file("empty").string(), dir.file("d.out").string()}).code, kExitOk);
  CHECK_EQ(fs::file_size(dir.file("d.out")), std::uintmax_t{0});
  const Outcome piped = run({"decode", "-", "-"}, "");
  CHECK_EQ(piped.code, kExitOk);
  CHECK_EQ(piped.out, std::string());
}

SYMENC_TEST(decode_errors_exit_one_and_report_code_and_offset) {
  TempDir dir;
  write_file(dir.file("bad"), symtest::repeat(symtest::kEuroText, 2));
  const Outcome o = run({"decode", dir.file("bad").string(), dir.file("out").string()});
  CHECK_EQ(o.code, kExitProtocolError);
  CHECK_EQ(o.err, std::string("symenc: E_TRUNCATED at byte offset 6\n"));
  CHECK_EQ(o.out, std::string());
}

SYMENC_TEST(encode_errors_exit_one_and_report_the_source_offset) {
  TempDir dir;
  write_file(dir.file("src"), "hello" + symtest::text_of({0x80}) + "x");
  const Outcome o = run({"encode", dir.file("src").string(), dir.file("out").string()});
  CHECK_EQ(o.code, kExitProtocolError);
  CHECK_EQ(o.err, std::string("symenc: E_NONASCII_INPUT at byte offset 5\n"));
}

SYMENC_TEST(A1_a_failed_decode_leaves_an_existing_destination_untouched) {
  TempDir dir;
  write_file(dir.file("bad"), "JIGX");
  write_file(dir.file("dest"), "KEEP");
  const Outcome o = run({"decode", dir.file("bad").string(), dir.file("dest").string()});
  CHECK_EQ(o.code, kExitProtocolError);
  CHECK_EQ(read_file(dir.file("dest")), std::string("KEEP"));
  CHECK_EQ(entries_in(dir.path()), std::size_t{2});
}

SYMENC_TEST(A2_a_failed_run_does_not_create_the_destination) {
  TempDir dir;
  write_file(dir.file("bad"), "JIGX");
  CHECK_EQ(run({"decode", dir.file("bad").string(), dir.file("dest").string()}).code, kExitProtocolError);
  CHECK(!fs::exists(dir.file("dest")));
  write_file(dir.file("nonascii"), symtest::text_of({0xC3, 0xA9}));
  CHECK_EQ(run({"encode", dir.file("nonascii").string(), dir.file("dest2").string()}).code, kExitProtocolError);
  CHECK(!fs::exists(dir.file("dest2")));
  CHECK_EQ(entries_in(dir.path()), std::size_t{2});
}

SYMENC_TEST(A3_a_failing_input_writes_zero_bytes_to_stdout) {
  const Outcome o = run({"decode", "-", "-"}, "SAW" + symtest::text_of({0x80}));
  CHECK_EQ(o.code, kExitProtocolError);
  CHECK_EQ(o.out.size(), std::size_t{0});
  const Outcome e = run({"encode", "-", "-"}, "ok" + symtest::text_of({0xFF}));
  CHECK_EQ(e.code, kExitProtocolError);
  CHECK_EQ(e.out.size(), std::size_t{0});
}

SYMENC_TEST(A4_success_leaves_the_exact_output_and_no_temporary_file) {
  TempDir dir;
  write_file(dir.file("src"), "Hello, World!\n");
  CHECK_EQ(run({"encode", dir.file("src").string(), dir.file("enc").string()}).code, kExitOk);
  CHECK_EQ(run({"decode", dir.file("enc").string(), dir.file("back").string()}).code, kExitOk);
  CHECK_EQ(read_file(dir.file("back")), std::string("Hello, World!\n"));
  CHECK_EQ(entries_in(dir.path()), std::size_t{3});
}

SYMENC_TEST(a_missing_input_file_exits_three) {
  TempDir dir;
  const Outcome o = run({"encode", dir.file("missing").string(), dir.file("out").string()});
  CHECK_EQ(o.code, kExitIo);
  CHECK(!fs::exists(dir.file("out")));
}

SYMENC_TEST(an_unwritable_destination_exits_three) {
  TempDir dir;
  write_file(dir.file("src"), "x");
  const Outcome o = run({"encode", dir.file("src").string(), dir.file("no/such/dir/out").string()});
  CHECK_EQ(o.code, kExitIo);
}

SYMENC_TEST(a_failing_stdout_is_an_io_error_not_a_protocol_error) {
  std::istringstream in("abc");
  std::ostringstream out;
  out.setstate(std::ios::badbit);
  std::ostringstream err;
  CHECK_EQ(symenc::cli::run({"encode", "-", "-"}, in, out, err), kExitIo);
}
