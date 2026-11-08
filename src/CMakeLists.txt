add_library(symenc_core STATIC
  decoder.cpp
  encoder.cpp
  error.cpp
  io.cpp
  tokenizer.cpp
  version.cpp)
target_include_directories(symenc_core PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(symenc_core PRIVATE symenc_options)

add_library(symenc_cli STATIC cli/cli.cpp)
target_link_libraries(symenc_cli PUBLIC symenc_core PRIVATE symenc_options)

add_executable(symenc cli/main.cpp)
target_link_libraries(symenc PRIVATE symenc_cli symenc_options)
