#include "symenc/cli.hpp"

#include <filesystem>
#include <istream>
#include <new>
#include <ostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <utility>

#include "symenc/codec.hpp"
#include "symenc/io.hpp"
#include "symenc/version.hpp"

namespace symenc::cli {

namespace fs = std::filesystem;

std::string usage() {
  return "usage: symenc encode IN OUT\n"
         "       symenc decode IN OUT\n"
         "       symenc --version | --help\n"
         "IN and OUT may be '-' for standard input / output.\n";
}

namespace {

bool same_path(const std::string& a, const std::string& b) {
  if (fs::path(a).lexically_normal() == fs::path(b).lexically_normal()) return true;
  std::error_code ec;
  const bool equivalent = fs::equivalent(a, b, ec);
  return !ec && equivalent;
}

}

int run(const std::vector<std::string>& args, std::istream& in, std::ostream& out,
        std::ostream& err) {
  if (args.size() == 1 && args[0] == "--version") {
    out << "symenc (spec " << spec_version() << ")\n";
    return kExitOk;
  }
  if (args.size() == 1 && (args[0] == "--help" || args[0] == "-h")) {
    out << usage();
    return kExitOk;
  }
  if (args.size() != 3 || (args[0] != "encode" && args[0] != "decode")) {
    err << usage();
    return kExitUsage;
  }

  const bool encoding = args[0] == "encode";
  const std::string& in_name = args[1];
  const std::string& out_name = args[2];
  if (in_name != "-" && out_name != "-" && same_path(in_name, out_name)) {
    err << "symenc: input and output must be different files\n";
    return kExitUsage;
  }

  try {
    const Bytes input = (in_name == "-") ? read_binary(in) : read_binary(fs::path(in_name));

    std::string text;
    if (encoding) {
      auto result = encode(input);
      if (failed(result)) {
        err << "symenc: " << format(error_of(result)) << '\n';
        return kExitProtocolError;
      }
      text = value_of(std::move(result));
    } else {
      auto result = decode(as_text(input));
      if (failed(result)) {
        err << "symenc: " << format(error_of(result)) << '\n';
        return kExitProtocolError;
      }
      const Bytes decoded = value_of(std::move(result));
      text.assign(as_text(decoded));
    }

    if (out_name == "-") {
      write_binary(out, text);
    } else {
      write_binary_atomic(fs::path(out_name), text);
    }
    return kExitOk;
  } catch (const IoError& e) {
    err << "symenc: " << e.what() << '\n';
    return kExitIo;
  } catch (const std::bad_alloc&) {
    err << "symenc: out of memory\n";
    return kExitIo;
  } catch (const std::exception& e) {
    err << "symenc: " << e.what() << '\n';
    return kExitIo;
  }
}

}
