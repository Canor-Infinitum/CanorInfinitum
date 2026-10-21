# Canor Infinitum pure-C core (AGPL-3.0-or-later).
#
#   cmake -S source -B build -G Ninja -DCMAKE_C_COMPILER=gcc
#   cmake --build build
#   ctest --test-dir build --output-on-failure
#
# The library is integer-only, allocation-free C11 (AGENTS.md invariants 1-2).
# examples/canor_demo.c is tracked separately and not built here.
cmake_minimum_required(VERSION 3.20)
project(canor_infinitum VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  set(CANOR_WARNINGS -Wall -Wextra -Werror -pedantic)
else()
  message(FATAL_ERROR "canor: the core requires GCC or Clang (-Wall -Wextra -Werror -pedantic)")
endif()

# The MSVC runtime (Clang on Windows) flags ISO C fopen/sscanf as deprecated in
# favour of non-portable *_s variants; the core stays on ISO C.
if(WIN32)
  add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()

file(GLOB CANOR_CORE_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/canor_*.c)

add_library(canor_core STATIC ${CANOR_CORE_SOURCES})
target_include_directories(canor_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_options(canor_core PRIVATE ${CANOR_WARNINGS})

# The timing library's property type is fixed at compile time, so the interval
# profile is its own small library rather than a runtime switch. It is
# self-contained (timing + swar), so no symbol is defined twice at link time.
add_library(canor_timing_interval STATIC src/canor_timing.c src/canor_swar.c)
target_include_directories(canor_timing_interval PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_definitions(canor_timing_interval PUBLIC CANOR_TIMING_INTERVAL=1)
target_compile_options(canor_timing_interval PRIVATE ${CANOR_WARNINGS})

add_executable(canor_gen tools/canor_gen.c)
target_compile_options(canor_gen PRIVATE ${CANOR_WARNINGS})
target_link_libraries(canor_gen PRIVATE canor_core)

enable_testing()
foreach(t swar timing soul enum)
  add_executable(canor_test_${t} tests/canor_test_${t}.c)
  target_compile_options(canor_test_${t} PRIVATE ${CANOR_WARNINGS})
  target_link_libraries(canor_test_${t} PRIVATE canor_core)
  add_test(NAME canor_test_${t} COMMAND canor_test_${t})
endforeach()

add_executable(canor_test_timing_interval tests/canor_test_timing.c)
target_compile_options(canor_test_timing_interval PRIVATE ${CANOR_WARNINGS})
target_link_libraries(canor_test_timing_interval PRIVATE canor_timing_interval)
add_test(NAME canor_test_timing_interval COMMAND canor_test_timing_interval)

# End-to-end: the generator writes every <language>/<language>.fwh/.fwb pair
# into the build tree (digests are profile-defined and left unset).
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/fw)
add_test(NAME canor_gen_files
         COMMAND canor_gen --out ${CMAKE_CURRENT_BINARY_DIR}/fw --sample 8)
