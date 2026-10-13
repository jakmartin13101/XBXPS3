# XBXPS3 core: RPCS3's emulator (rpcs3_emu and its 3rdparty libraries) plus the Xbox
# frontend glue in src/, merged into a single static library, xbxps3_core.lib.
#
# This directory is not a standalone project. RPCS3 stays the top-level CMake project
# and adds this directory when configured with
#     -DRPCS3_UWP=ON -DXBXPS3_FRONTEND_DIR=<repo>/core
# (patches/rpcs3/0006). scripts/build.ps1 does that for you.
#
# Everything here is compiled as ordinary desktop Win32 code, like the XBSX2 and Dolphin
# Xbox ports do with their cores. Only app/ is a UWP project.

cmake_minimum_required(VERSION 3.28)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(XBXPS3_VOLK_DIR "${XBXPS3_FRONTEND_DIR}/../external/volk" CACHE PATH "volk checkout (scripts/bootstrap.ps1 fetches it)")
if(NOT EXISTS "${XBXPS3_VOLK_DIR}/volk.h")
	message(FATAL_ERROR "volk not found in ${XBXPS3_VOLK_DIR}. Run scripts/bootstrap.ps1 first.")
endif()

set(RPCS3_SRC "${CMAKE_SOURCE_DIR}/rpcs3")
set(XBX_GENERATED "${CMAKE_CURRENT_BINARY_DIR}/generated")

# Overlay strings, extracted from the Qt string table (see tools/gen_localized_strings.py)
add_custom_command(
	OUTPUT "${XBX_GENERATED}/xbx_localized_table.inc"
	COMMAND ${CMAKE_COMMAND} -E make_directory "${XBX_GENERATED}"
	COMMAND Python3::Interpreter -I "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_localized_strings.py"
		"${RPCS3_SRC}/rpcs3qt/localized_emu.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/tools/overrides.txt"
		"${XBX_GENERATED}/xbx_localized_table.inc"
	DEPENDS
		"${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_localized_strings.py"
		"${CMAKE_CURRENT_SOURCE_DIR}/tools/overrides.txt"
		"${RPCS3_SRC}/rpcs3qt/localized_emu.h"
	COMMENT "Generating overlay string table"
	VERBATIM)

add_library(xbxps3_frontend STATIC
	include/xbx_core.h
	src/xbx_internal.h
	src/xbx_core.cpp
	src/xbx_callbacks.cpp
	src/xbx_gs_frame.h
	src/xbx_gs_frame.cpp
	src/xbx_image.cpp
	src/xbx_install.cpp
	src/xbx_settings.cpp
	src/xbx_strings.cpp
	src/xbx_vulkan.cpp
	src/xbx_win32_shims.cpp
	"${XBX_GENERATED}/xbx_localized_table.inc"

	# Desktop-frontend sources the core depends on. Upstream compiles them into the Qt
	# library (rpcs3qt/CMakeLists.txt); patch 0005 makes them build without Qt.
	"${RPCS3_SRC}/rpcs3_version.cpp"
	"${RPCS3_SRC}/Input/camera_video_sink.cpp"
	"${RPCS3_SRC}/Input/ds3_pad_handler.cpp"
	"${RPCS3_SRC}/Input/ds4_pad_handler.cpp"
	"${RPCS3_SRC}/Input/dualsense_pad_handler.cpp"
	"${RPCS3_SRC}/Input/hid_instance.cpp"
	"${RPCS3_SRC}/Input/hid_pad_handler.cpp"
	"${RPCS3_SRC}/Input/mouse_gyro_handler.cpp"
	"${RPCS3_SRC}/Input/pad_thread.cpp"
	"${RPCS3_SRC}/Input/product_info.cpp"
	"${RPCS3_SRC}/Input/ps_move_calibration.cpp"
	"${RPCS3_SRC}/Input/ps_move_config.cpp"
	"${RPCS3_SRC}/Input/ps_move_handler.cpp"
	"${RPCS3_SRC}/Input/ps_move_tracker.cpp"
	"${RPCS3_SRC}/Input/raw_mouse_config.cpp"
	"${RPCS3_SRC}/Input/skateboard_pad_handler.cpp"
	"${RPCS3_SRC}/Input/wiimote_handler.cpp"
	"${RPCS3_SRC}/Input/xinput_pad_handler.cpp"
)

target_include_directories(xbxps3_frontend
	PUBLIC
		"${CMAKE_CURRENT_SOURCE_DIR}/include"
	PRIVATE
		"${CMAKE_CURRENT_SOURCE_DIR}/src"
		"${RPCS3_SRC}"
		"${XBX_GENERATED}"
		"${XBXPS3_VOLK_DIR}")

# The VK backend inside rpcs3_emu includes <volk.h> as well (patch 0003)
target_include_directories(rpcs3_emu PRIVATE "${XBXPS3_VOLK_DIR}")

target_compile_definitions(xbxps3_frontend PRIVATE UNICODE _UNICODE VK_USE_PLATFORM_WIN32_KHR)
target_compile_options(xbxps3_frontend PRIVATE /utf-8)

target_link_libraries(xbxps3_frontend
	PUBLIC
		rpcs3_emu
	PRIVATE
		3rdparty::stblib
		3rdparty::libpng
		3rdparty::hidapi
		3rdparty::opencv
		3rdparty::fusion
		3rdparty::wolfssl
		3rdparty::libcurl
		3rdparty::zlib)

if(USE_PRECOMPILED_HEADERS)
	target_precompile_headers(xbxps3_frontend PRIVATE "${RPCS3_SRC}/stdafx.h")
endif()

# Merge xbxps3_frontend and every static library it pulls in into one .lib. The UWP app
# then needs exactly one library from this build, plus the system import libraries listed
# in xbxps3_core_system_libs.txt (filtered against an allowlist in app/CMakeLists.txt).
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/XbxMergeStaticLibs.cmake")
xbx_merge_static_libs(xbxps3_core xbxps3_frontend)
