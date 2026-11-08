add_library(symenc_testsupport STATIC support/test_main.cpp)
target_include_directories(symenc_testsupport PUBLIC support)
target_link_libraries(symenc_testsupport PRIVATE symenc_options)

function(symenc_add_test name)
  add_executable(${name} ${ARGN})
  target_link_libraries(${name} PRIVATE symenc_cli symenc_core symenc_testsupport symenc_options)
  add_test(NAME ${name} COMMAND ${name})
endfunction()

symenc_add_test(test_smoke test_smoke.cpp)
symenc_add_test(test_error test_error.cpp)
symenc_add_test(test_tokenizer test_tokenizer.cpp)
symenc_add_test(test_encoder test_encoder.cpp)
symenc_add_test(test_encoder_ascii test_encoder_ascii.cpp)
symenc_add_test(test_decoder test_decoder.cpp)
symenc_add_test(test_roundtrip test_roundtrip.cpp)
symenc_add_test(test_io test_io.cpp)
symenc_add_test(test_cli test_cli.cpp)
target_compile_definitions(test_cli PRIVATE SYMENC_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")

symenc_add_test(test_harness_detects_failure test_harness_failure.cpp)
set_tests_properties(test_harness_detects_failure PROPERTIES WILL_FAIL TRUE)

add_test(NAME cli_version COMMAND symenc --version)
set_tests_properties(cli_version PROPERTIES PASS_REGULAR_EXPRESSION "spec 0\\.1\\.1")

add_test(NAME cli_e2e
  COMMAND ${CMAKE_COMMAND}
    -DSYMENC=$<TARGET_FILE:symenc>
    -DDATA_DIR=${CMAKE_CURRENT_SOURCE_DIR}/data
    -DWORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/e2e
    -P ${CMAKE_CURRENT_SOURCE_DIR}/cli_e2e.cmake)
