# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

#find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

# Re-direct the directory where the 'boards' directory is found from
# $ZEPHYR_BASE to this directory.
set(BOARD_ROOT ${CMAKE_CURRENT_LIST_DIR})

# This sample is only compatible with walterneo because it has
# re-directed the 'boards' directory to a directory that only contains
# this board.
set(BOARD walterneo)

include($ENV{ZEPHYR_BASE}/cmake/app/boilerplate.cmake NO_POLICY_SCOPE)

project(mesh_demo)

FILE(GLOB app_sources src/*.c)
target_sources(app PRIVATE ${app_sources})
FILE(GLOB mesh_sources src/mesh/*.c)
target_sources(app PRIVATE ${mesh_sources})
FILE(GLOB misc_sources src/misc/*.c)
target_sources(app PRIVATE ${misc_sources})
FILE(GLOB sensor_sources src/sensor/*.c)
target_sources(app PRIVATE ${sensor_sources})

if(NODE_ADDR)
  zephyr_compile_definitions(NODE_ADDR=${NODE_ADDR})
endif()
