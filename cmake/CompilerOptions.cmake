add_library(symenc_options INTERFACE)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(symenc_options INTERFACE
    -Wall -Wextra -Wpedantic
    -Wconversion -Wsign-conversion
    -Wshadow -Wold-style-cast -Wcast-align
    -Wnon-virtual-dtor -Woverloaded-virtual -Wformat=2)
  if(SYMENC_WERROR)
    target_compile_options(symenc_options INTERFACE -Werror)
  endif()
  if(SYMENC_SANITIZE)
    target_compile_options(symenc_options INTERFACE
      -fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer)
    target_link_options(symenc_options INTERFACE -fsanitize=address,undefined)
  endif()
elseif(MSVC)
  target_compile_options(symenc_options INTERFACE /W4 /permissive- /utf-8)
  if(SYMENC_WERROR)
    target_compile_options(symenc_options INTERFACE /WX)
  endif()
  if(SYMENC_SANITIZE)
    message(WARNING "SYMENC_SANITIZE: MSVC supports AddressSanitizer only, not UBSan.")
    target_compile_options(symenc_options INTERFACE /fsanitize=address)
  endif()
else()
  message(WARNING "Unknown compiler '${CMAKE_CXX_COMPILER_ID}': no warning flags applied.")
endif()
