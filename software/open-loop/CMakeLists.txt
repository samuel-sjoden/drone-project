cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
idf_build_set_property(MINIMAL_BUILD ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
project(open-loop)
