cmake_minimum_required(VERSION 3.20)
project(falcon-dsl VERSION 1.0.2 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_TESTS  "Build tests"                         ON)
option(USE_CCACHE   "Enable ccache or sccache if available" ON)
option(ENABLE_PCH   "Enable target_precompile_headers"    ON)

if(USE_CCACHE)
  find_program(SCCACHE_PROGRAM sccache)
  if(SCCACHE_PROGRAM)
    set(CMAKE_C_COMPILER_LAUNCHER   ${SCCACHE_PROGRAM})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${SCCACHE_PROGRAM})
  else()
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
      set(CMAKE_C_COMPILER_LAUNCHER   ${CCACHE_PROGRAM})
      set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
    endif()
  endif()
endif()

find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog        CONFIG REQUIRED)
find_package(falcon-typing CONFIG REQUIRED)
find_package(falcon-package-manager CONFIG REQUIRED)
find_package(fmt           CONFIG REQUIRED)
find_package(OpenSSL       CONFIG REQUIRED)

# 1. Compiler sub-project
add_subdirectory(compiler)

# 3. DSL Runtime Library (falcon-dsl)
add_library(falcon-dsl SHARED
  src/AutotunerEngine.cpp
  src/FunctionRegistry.cpp
  src/StmtExecutor.cpp
  src/TypeRegistry.cpp
  src/ExprEvaluator.cpp
  src/Interpreter.cpp
  src/SourceContext.cpp
  src/log.cpp
)

target_include_directories(falcon-dsl
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries(falcon-dsl
  PUBLIC
    nlohmann_json::nlohmann_json
    falcon-atc
    falcon::falcon-typing
  PRIVATE
    falcon::falcon-package-manager
    fmt::fmt
    OpenSSL::SSL
)

# ── 4. CLI tool (falcon-run) ─────────────────────────────────────────────────
add_executable(falcon-run src/cli/main.cpp)
target_link_libraries(falcon-run PRIVATE falcon-dsl)
target_include_directories(falcon-run
    PRIVATE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  )
install(TARGETS falcon-run RUNTIME DESTINATION bin)
# falcon-test: fixture-aware test runner
# Reads user .fal test files, injects harness states, and executes all
# discovered test suite autotuners.
add_executable(falcon-test src/cli/test_main.cpp)
target_link_libraries(falcon-test PRIVATE falcon-dsl)
target_include_directories(falcon-test
    PRIVATE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  )
install(TARGETS falcon-test RUNTIME DESTINATION bin)

target_compile_options(falcon-dsl PRIVATE -Wall -Wextra -Wpedantic)

if(ENABLE_PCH)
  target_precompile_headers(falcon-dsl
    PRIVATE
      <string> <vector> <memory> <variant> <optional>
      <nlohmann/json.hpp>
  )
endif()

if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

# ── Install ───────────────────────────────────────────────────────────────────
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS falcon-dsl
  EXPORT falcon-dsl-targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install CLI separately WITHOUT exporting it
install(TARGETS falcon-run falcon-test
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/falcon-dsl
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILES_MATCHING
    PATTERN "*.hpp"
    PATTERN "*.h"
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/falcon-dsl-config-version.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

install(EXPORT falcon-dsl-targets
  FILE      falcon-dsl-targets.cmake
  NAMESPACE falcon::
  DESTINATION ${CMAKE_INSTALL_DATADIR}/falcon-dsl
)
configure_package_config_file(
  cmake/falcon-dsl-config.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/falcon-dsl-config.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_DATADIR}/falcon-dsl
)
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/falcon-dsl-config.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/falcon-dsl-config-version.cmake"
  DESTINATION ${CMAKE_INSTALL_DATADIR}/falcon-dsl
)
