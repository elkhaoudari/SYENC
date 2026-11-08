#include <iostream>
#include <string>
#include <vector>

#include "symenc/cli.hpp"

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

int main(int argc, char** argv) {
#ifdef _WIN32
  _setmode(_fileno(stdin), _O_BINARY);
  _setmode(_fileno(stdout), _O_BINARY);
#endif
  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i) args.emplace_back(argv[i]);
  return symenc::cli::run(args, std::cin, std::cout, std::cerr);
}
