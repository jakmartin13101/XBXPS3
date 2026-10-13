# xbx_merge_static_libs(<output_target> <root_target>)
#
# Walks the link graph of <root_target>, merges every static library it reaches (its own
# targets, absolute .lib paths such as the prebuilt ffmpeg, and STATIC imported targets)
# into ${CMAKE_BINARY_DIR}/xbxps3_core/xbxps3_core.lib with lib.exe, and writes the plain
# system library names it found (ws2_32, d3d12, ...) to xbxps3_core_system_libs.txt.
#
# Must be called after every target in the graph has been created.

function(_xbx_strip_genex item out_var)
	# $<LINK_ONLY:x> and $<BUILD_INTERFACE:x> keep x; other generator expressions are dropped
	set(value "${item}")
	while(value MATCHES "^\\$<(LINK_ONLY|BUILD_INTERFACE|BUILD_LOCAL_INTERFACE):(.*)>$")
		set(value "${CMAKE_MATCH_2}")
	endwhile()
	if(value MATCHES "^\\$<")
		set(value "")
	endif()
	set(${out_var} "${value}" PARENT_SCOPE)
endfunction()

function(xbx_merge_static_libs output_target root_target)
	set(stack "${root_target}")
	set(visited "")
	set(lib_targets "")
	set(lib_files "")
	set(system_libs "")

	while(stack)
		list(POP_FRONT stack current)

		if(current STREQUAL "" OR current IN_LIST visited)
			continue()
		endif()
		list(APPEND visited "${current}")

		if(TARGET "${current}")
			get_target_property(aliased "${current}" ALIASED_TARGET)
			if(aliased)
				list(APPEND stack "${aliased}")
				continue()
			endif()

			get_target_property(type "${current}" TYPE)
			get_target_property(imported "${current}" IMPORTED)

			if(type STREQUAL "STATIC_LIBRARY")
				if(imported)
					get_target_property(location "${current}" IMPORTED_LOCATION_RELEASE)
					if(NOT location)
						get_target_property(location "${current}" IMPORTED_LOCATION)
					endif()
					if(location)
						list(APPEND lib_files "${location}")
					endif()
				else()
					list(APPEND lib_targets "${current}")
				endif()
			elseif(type STREQUAL "SHARED_LIBRARY" OR type STREQUAL "MODULE_LIBRARY")
				message(WARNING "xbx_merge_static_libs: ${current} is a shared library and cannot be merged. Build it static for the Xbox core.")
			endif()

			set(props INTERFACE_LINK_LIBRARIES)
			if(NOT imported AND NOT type STREQUAL "INTERFACE_LIBRARY")
				list(APPEND props LINK_LIBRARIES)
			endif()

			foreach(prop IN LISTS props)
				get_target_property(deps "${current}" ${prop})
				if(deps)
					foreach(dep IN LISTS deps)
						_xbx_strip_genex("${dep}" dep)
						if(dep AND NOT dep MATCHES "^-")
							list(APPEND stack "${dep}")
						endif()
					endforeach()
				endif()
			endforeach()
		elseif(IS_ABSOLUTE "${current}")
			if(current MATCHES "\\.lib$")
				list(APPEND lib_files "${current}")
			endif()
		elseif(NOT current MATCHES "::")
			# Plain name: a system import library (ws2_32, dxgi, ...)
			string(REGEX REPLACE "\\.lib$" "" name "${current}")
			list(APPEND system_libs "${name}")
		endif()
	endwhile()

	list(REMOVE_DUPLICATES lib_targets)
	list(REMOVE_DUPLICATES lib_files)
	list(REMOVE_DUPLICATES system_libs)

	set(out_dir "${CMAKE_BINARY_DIR}/xbxps3_core")
	set(out_lib "${out_dir}/xbxps3_core.lib")
	set(rsp "${out_dir}/xbxps3_core_merge.rsp")

	set(rsp_lines "")
	foreach(t IN LISTS lib_targets)
		string(APPEND rsp_lines "\"$<TARGET_LINKER_FILE:${t}>\"\n")
	endforeach()
	foreach(f IN LISTS lib_files)
		string(APPEND rsp_lines "\"${f}\"\n")
	endforeach()

	file(GENERATE OUTPUT "${rsp}" CONTENT "${rsp_lines}")

	list(JOIN system_libs "\n" system_text)
	file(WRITE "${out_dir}/xbxps3_core_system_libs.txt" "${system_text}\n")

	add_custom_command(
		OUTPUT "${out_lib}"
		COMMAND "${CMAKE_AR}" /NOLOGO /IGNORE:4006 /IGNORE:4221 "/OUT:${out_lib}" "@${rsp}"
		DEPENDS ${lib_targets} "${rsp}"
		COMMENT "Merging ${root_target} and its dependencies into xbxps3_core.lib"
		VERBATIM)

	add_custom_target(${output_target} ALL DEPENDS "${out_lib}")

	list(LENGTH lib_targets n_targets)
	list(LENGTH lib_files n_files)
	message(STATUS "XBXPS3: merging ${n_targets} targets and ${n_files} prebuilt libraries into xbxps3_core.lib")
	message(STATUS "XBXPS3: system libraries requested by the core: ${system_libs}")
endfunction()
