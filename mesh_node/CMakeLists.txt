cmake_minimum_required(VERSION 3.16)

# Make the shared components/ visible to this app
set(EXTRA_COMPONENT_DIRS "${CMAKE_SOURCE_DIR}/../components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(mesh_node)
