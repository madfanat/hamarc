include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/releases/download/v1.16.0/googletest-1.16.0.tar.gz
  URL_HASH SHA256=78c676fc63881529bf97bf9d45948d905a66833fbfa5318ea2cd7478cb98f399
  FIND_PACKAGE_ARGS NAMES GTest
)

if(MSVC)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
endif()

set(BUILD_GMOCK OFF)
FetchContent_MakeAvailable(googletest)

add_executable(hamarc_tests test_archiver.cc test_errors.cc)

target_include_directories(hamarc_tests PRIVATE "${PROJECT_SOURCE_DIR}/src")
add_dependencies(hamarc_tests hamarc)

target_link_libraries(hamarc_tests PRIVATE GTest::gtest_main)

target_compile_definitions(
    hamarc_tests
    PRIVATE
        RESOURCES_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/resources\"
        HAMARC_EXE_PATH=\"$<TARGET_FILE:hamarc>\"
)

include(GoogleTest)
gtest_discover_tests(hamarc_tests)
