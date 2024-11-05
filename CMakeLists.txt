cmake_minimum_required(VERSION 3.22)

project(VocalDivider)

set(CMAKE_CXX_STANDARD 20)
set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)

## *********** COMMENT-OUT IF USING CPM ************
set(JUCE_DIR "C:/Users/user/Desktop/UF0/_JUCE/JUCE")
add_subdirectory("${JUCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE_build" EXCLUDE_FROM_ALL)
list(APPEND CMAKE_MODULE_PATH "${JUCE_DIR}/modules")
## *************************************************

## *** COMMENT-OUT IF USING LOCAL JUCE LIB ***
# include(cmake/cpm.cmake)
# CPMAddPackage(
#     NAME JUCE
#     GITHUB_REPOSITORY juce-framework/JUCE
#     GIT_TAG 8.0.3
#     VERSION 8.0.3
#     SOURCE_DIR ${LIB_DIR}/juce
# )
## *******************************************

add_subdirectory(plugin)