#pragma once

#include <iosfwd>
#include <string>
#include <vector>

namespace symenc::cli {

inline constexpr int kExitOk = 0;
inline constexpr int kExitProtocolError = 1;
inline constexpr int kExitUsage = 2;
inline constexpr int kExitIo = 3;

[[nodiscard]] std::string usage();

int run(const std::vector<std::string>& args, std::istream& in, std::ostream& out,
        std::ostream& err);

}
