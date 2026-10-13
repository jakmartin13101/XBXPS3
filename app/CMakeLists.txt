# XBXPS3 UWP app (stage 2). Configure with the Visual Studio generator for WindowsStore:
#
#   cmake -S app -B build/app -G "Visual Studio 17 2022" -A x64 ^
#         -DCMAKE_SYSTEM_NAME=WindowsStore -DCMAKE_SYSTEM_VERSION=10.0.22621.0 ^
#         -DXBXPS3_CORE_DIR=build/core/xbxps3_core -DXBXPS3_DOZEN_DLL=build/mesa/vulkan_dzn.dll ^
#         -DXBXPS3_CERT_PFX=build/XBXPS3_TemporaryKey.pfx
#
# scripts/build.ps1 runs both stages. Only the Release configuration is supported, because
# the core is built Release and both halves must share the same app CRT.

cmake_minimum_required(VERSION 3.28)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "WindowsStore")
	message(FATAL_ERROR "Configure the app with -DCMAKE_SYSTEM_NAME=WindowsStore (see the comment at the top of this file)")
endif()

project(XBXPS3 LANGUAGES CXX)

set(XBXPS3_CORE_DIR "" CACHE PATH "Folder with xbxps3_core.lib and xbxps3_core_system_libs.txt (stage 1 output)")
set(XBXPS3_DOZEN_DLL "" CACHE FILEPATH "vulkan_dzn.dll built from Mesa-UWP (scripts/build-mesa.ps1)")
set(XBXPS3_CERT_PFX "" CACHE FILEPATH "Signing certificate whose subject is CN=XBXPS3 Dev (scripts/build.ps1 creates one)")
set(XBXPS3_RPCS3_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../external/rpcs3" CACHE PATH "Patched RPCS3 checkout (for bin/Icons)")
set(XBXPS3_FONTS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../external/fonts" CACHE PATH "TTF fonts for the overlays until PS3 firmware is installed")

foreach(required XBXPS3_CORE_DIR XBXPS3_DOZEN_DLL XBXPS3_CERT_PFX)
	if(NOT ${required})
		message(FATAL_ERROR "${required} is not set")
	endif()
endforeach()

set(CORE_LIB "${XBXPS3_CORE_DIR}/xbxps3_core.lib")
if(NOT EXISTS "${CORE_LIB}")
	message(FATAL_ERROR "${CORE_LIB} not found. Build the core first (scripts/build.ps1 -Stage core).")
endif()

# --- Packaged content -----------------------------------------------------------------------

set(content_files "")

# Content stays at its path relative to <base_dir>, under <package_dir> inside the package
function(xbx_add_content base_dir package_dir)
	file(GLOB_RECURSE files RELATIVE "${base_dir}" "${base_dir}/*")
	foreach(rel IN LISTS files)
		get_filename_component(rel_dir "${rel}" DIRECTORY)
		set(location "${package_dir}")
		if(rel_dir)
			set(location "${package_dir}/${rel_dir}")
		endif()
		set(abs "${base_dir}/${rel}")
		set_source_files_properties("${abs}" PROPERTIES VS_DEPLOYMENT_CONTENT 1 VS_DEPLOYMENT_LOCATION "${location}")
		list(APPEND content_files "${abs}")
	endforeach()
	set(content_files "${content_files}" PARENT_SCOPE)
endfunction()

xbx_add_content("${CMAKE_CURRENT_SOURCE_DIR}/Assets" "Assets")
xbx_add_content("${XBXPS3_RPCS3_DIR}/bin/Icons/ui" "Icons/ui") # Overlay button glyphs and icons
if(EXISTS "${XBXPS3_FONTS_DIR}")
	xbx_add_content("${XBXPS3_FONTS_DIR}" "Fonts")
endif()

# The Vulkan driver, loaded with LoadPackagedLibrary at startup, and the DXIL validator
# Dozen uses to sign the shaders it generates (from the Windows SDK bin folder)
set(XBXPS3_DXIL_DLL "" CACHE FILEPATH "dxil.dll to package next to vulkan_dzn.dll")
foreach(dll IN ITEMS "${XBXPS3_DOZEN_DLL}" "${XBXPS3_DXIL_DLL}")
	if(dll)
		set_source_files_properties("${dll}" PROPERTIES VS_DEPLOYMENT_CONTENT 1 VS_DEPLOYMENT_LOCATION ".")
		list(APPEND content_files "${dll}")
	endif()
endforeach()

# CMake uses a .pfx listed in the sources as the package signing key
configure_file("${XBXPS3_CERT_PFX}" "${CMAKE_CURRENT_BINARY_DIR}/XBXPS3_TemporaryKey.pfx" COPYONLY)

add_executable(XBXPS3 WIN32
	src/App.cpp
	src/pch.h
	../core/include/xbx_core.h
	Package.appxmanifest
	"${CMAKE_CURRENT_BINARY_DIR}/XBXPS3_TemporaryKey.pfx"
	${content_files})

target_include_directories(XBXPS3 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../core/include")
target_precompile_headers(XBXPS3 PRIVATE src/pch.h)
target_compile_features(XBXPS3 PRIVATE cxx_std_20)
target_compile_options(XBXPS3 PRIVATE /bigobj /EHsc /utf-8 /permissive-)
target_compile_definitions(XBXPS3 PRIVATE UNICODE _UNICODE)

# --- Linking ---------------------------------------------------------------------------------
#
# Only libraries whose DLLs exist on the Xbox OS may be linked. A desktop import library
# (user32, gdi32, setupapi, winmm, hid, ...) adds a DLL import that makes the process fail to
# start. The core asks for whatever its desktop dependencies want; we keep the safe ones and
# let WindowsApp.lib + mincore.lib supply the rest. scripts/check-imports.ps1 audits the result.

set(XBX_ALLOWED_SYSTEM_LIBS ws2_32 bcrypt crypt32 d3d12 dxgi d2d1 dwrite dxguid xaudio2 synchronization)

file(STRINGS "${XBXPS3_CORE_DIR}/xbxps3_core_system_libs.txt" core_system_libs)
set(system_libs "")
set(dropped_libs "")
foreach(lib IN LISTS core_system_libs)
	string(TOLOWER "${lib}" lib_lower)
	if(lib_lower IN_LIST XBX_ALLOWED_SYSTEM_LIBS)
		list(APPEND system_libs "${lib_lower}.lib")
	elseif(lib)
		list(APPEND dropped_libs "${lib}")
	endif()
endforeach()
message(STATUS "XBXPS3: system libraries linked: ${system_libs}")
message(STATUS "XBXPS3: system libraries dropped (resolved by WindowsApp/mincore, or link triage): ${dropped_libs}")

target_link_libraries(XBXPS3 PRIVATE
	"${CORE_LIB}"
	${system_libs}
	WindowsApp.lib
	mincore.lib
	xinputuap.lib
	xaudio2.lib
	d3d12.lib
	dxgi.lib)

# Desktop import libraries that #pragma comment(lib) in 3rdparty code may still request
set(XBX_FORBIDDEN_DEFAULTLIBS user32 gdi32 setupapi winmm hid cfgmgr32 iphlpapi shell32 comdlg32 opengl32 vulkan-1 xinput xinput9_1_0 dbghelp psapi)
foreach(lib IN LISTS XBX_FORBIDDEN_DEFAULTLIBS)
	target_link_options(XBXPS3 PRIVATE "/NODEFAULTLIB:${lib}.lib")
endforeach()

# RPCS3 needs a large main stack (upstream links with /STACK:8388608)
target_link_options(XBXPS3 PRIVATE /STACK:8388608,1048576 /IGNORE:4099)

# --- Packaging -------------------------------------------------------------------------------

set_target_properties(XBXPS3 PROPERTIES
	VS_GLOBAL_GenerateAppxPackageOnBuild "true"
	VS_GLOBAL_AppxBundle "Never"
	VS_GLOBAL_AppxPackageDir "${CMAKE_BINARY_DIR}/AppPackages/"
	VS_GLOBAL_AppxPackageSigningEnabled "true"
	VS_GLOBAL_PackageCertificatePassword "xbxps3" # Matches scripts/build.ps1; this key only signs sideloaded dev builds
	VS_GLOBAL_UapAppxPackageBuildMode "SideloadOnly"
	VS_GLOBAL_AppxSymbolPackageEnabled "false")

# Fail the build early if a forbidden DLL import slipped through
add_custom_command(TARGET XBXPS3 POST_BUILD
	COMMAND powershell -NoProfile -ExecutionPolicy Bypass -File "${CMAKE_CURRENT_SOURCE_DIR}/../scripts/check-imports.ps1" -Binary "$<TARGET_FILE:XBXPS3>"
	VERBATIM)
