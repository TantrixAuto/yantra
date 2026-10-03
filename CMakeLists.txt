cmake_minimum_required(VERSION 3.23)
set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin")

# derive the project version from the nearest git tag (e.g. v0.5.0), unless
# a packager already passed one in: source tarballs (AUR, Homebrew, Fedora,
# Debian, ...) have no .git directory at all, so git describe can't work
# there, not just in CI's shallow clones.
if(NOT DEFINED YANTRA_VERSION)
  find_package(Git QUIET)
  if(Git_FOUND)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      OUTPUT_VARIABLE GIT_VERSION_TAG
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET
    )
  endif()

  if(GIT_VERSION_TAG MATCHES "^v?([0-9]+)\\.([0-9]+)\\.([0-9]+)")
    set(YANTRA_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
  else()
    set(YANTRA_VERSION "0.0.0")
  endif()
endif()

include(yantra)
project(yantra LANGUAGES CXX VERSION ${YANTRA_VERSION})

add_subdirectory(src)
