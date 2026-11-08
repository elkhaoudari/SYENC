#pragma once

#include <ostream>

#include "symenc/error.hpp"

namespace symenc {

inline std::ostream& operator<<(std::ostream& out, ErrorCode code) { return out << name(code); }
inline std::ostream& operator<<(std::ostream& out, const Error& error) { return out << format(error); }

}
