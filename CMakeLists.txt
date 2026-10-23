cmake_minimum_required(VERSION 3.24)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  # Forbid in-tree builds (which clobber Makefile)
  message(FATAL_ERROR "Build in a separate directory: cmake -S . -B build")
endif()

project(nvcx VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_BUILD_TYPE RelWithDebInfo)
include(GNUInstallDirs)

add_library(nvcx_warnings INTERFACE)
target_compile_options(nvcx_warnings INTERFACE -Wall -Wextra -Wno-missing-field-initializers -Werror)

# Core (the kernel runs on a Boost.Context fiber)
find_package(Boost CONFIG REQUIRED COMPONENTS context)
add_library(nvcx_core STATIC src/core/kernel.cpp src/core/vivado.cpp src/core/simulation.cpp)
target_link_libraries(nvcx_core PUBLIC ${CMAKE_DL_LIBS} Boost::context PRIVATE nvcx_warnings)
target_include_directories(nvcx_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# VHPI plugin for NVC
add_library(nvcx_plugin SHARED src/bridge/bridge.cpp)
target_link_libraries(nvcx_plugin PRIVATE nvcx_core nvcx_warnings)
target_link_options(nvcx_plugin PRIVATE "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/src/bridge/exports.map" -Wl,-Bsymbolic)
set_target_properties(nvcx_plugin PROPERTIES OUTPUT_NAME nvcx LINK_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/src/bridge/exports.map)

# WDB stub to reduce Vivado's shared-library blast radius
add_library(wdbstub SHARED src/wdbstub.cpp)
set_target_properties(wdbstub PROPERTIES OUTPUT_NAME xv_simbridge_kernel)

# the core's VHDL entity, beside the build tree's plugin
add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/nvcx.vhd
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/vhdl/nvcx.vhd ${CMAKE_BINARY_DIR}/nvcx.vhd
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/vhdl/nvcx.vhd)
add_custom_target(nvcx_vhd ALL DEPENDS ${CMAKE_BINARY_DIR}/nvcx.vhd)

# Tcl scripts for `nvc --do`, beside the build tree's executable
file(GLOB NVCX_TCL CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/tcl/*.tcl)
add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/tcl.stamp
  COMMAND ${CMAKE_COMMAND} -E rm -rf ${CMAKE_BINARY_DIR}/tcl
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/tcl
  COMMAND ${CMAKE_COMMAND} -E copy ${NVCX_TCL} ${CMAKE_BINARY_DIR}/tcl
  COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_BINARY_DIR}/tcl.stamp
  DEPENDS ${NVCX_TCL})
add_custom_target(nvcx_tcl ALL DEPENDS ${CMAKE_BINARY_DIR}/tcl.stamp)

# Core tests
add_executable(nvcx-tests tests/engine_tests.cpp)
target_link_libraries(nvcx-tests PRIVATE nvcx_core nvcx_warnings)

install(TARGETS nvcx_plugin wdbstub LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/nvcx)
install(FILES vhdl/nvcx.vhd DESTINATION ${CMAKE_INSTALL_DATADIR}/nvcx)
install(DIRECTORY tcl/ DESTINATION ${CMAKE_INSTALL_DATADIR}/nvcx/tcl)
