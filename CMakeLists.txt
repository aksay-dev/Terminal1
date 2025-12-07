# The following lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)
include($ENV{IDF_PATH}/tools/cmake/version.cmake)

# Add newly added components to one of these lines:
# 1. Add here if the component is compatible with IDF >= v4.3

set(TEST_COMPONENTS "")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Terminal1)

# Ensure LVGL can find project lv_conf.h from the repo root
idf_build_set_property(INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}" APPEND)
idf_build_set_property(COMPILE_DEFINITIONS "LV_CONF_PATH=\"${CMAKE_SOURCE_DIR}/lv_conf.h\"" APPEND)
