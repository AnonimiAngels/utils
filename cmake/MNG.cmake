cmake_minimum_required(VERSION 3.30)

find_package(Git REQUIRED)

set(Python3_ROOT_DIR "/usr")
find_package(Python3 REQUIRED COMPONENTS Interpreter)

if(NOT DEFINED mng_source_cache)
	set(mng_source_cache "${CMAKE_CURRENT_SOURCE_DIR}/deps" CACHE PATH "Directory to cache downloaded packages")
endif()

file(MAKE_DIRECTORY "${mng_source_cache}")

set(_mng_impl_script "${CMAKE_CURRENT_LIST_DIR}/mng_impl.py")

function(_mng_run)
	set(_mng_command ${Python3_EXECUTABLE})
	list(APPEND _mng_command -u "${_mng_impl_script}")
	list(APPEND _mng_command ${ARGN})
	string(JOIN " " _mng_command_str ${_mng_command})


	execute_process(
		COMMAND ${_mng_command}
		RESULT_VARIABLE _mng_result
		COMMAND_ERROR_IS_FATAL ANY
	)

	if(NOT _mng_result EQUAL 0)
		message(FATAL_ERROR
			"MNG: command failed with exit code ${_mng_result}\n"
			"stdout:\n${_mng_stdout}\n\n"
			"stderr:\n${_mng_stderr}\n"
		)
	endif()
endfunction()

function(_mng_add_subdirectory p_name p_dir p_exclude p_system p_verbose)
	set(build_dir "${CMAKE_BINARY_DIR}/_mng/${p_name}")

	if(NOT p_verbose)
		set(p_verbose "WARNING")
	endif()
	set(mng_saved_verbose "${CMAKE_MESSAGE_LOG_LEVEL}")
	set(CMAKE_MESSAGE_LOG_LEVEL "${p_verbose}")

	set(${p_name}_SOURCE_DIR "${build_dir}" CACHE PATH "" FORCE)

	if(p_exclude)
		if(p_system)
			add_subdirectory("${p_dir}" "${build_dir}" EXCLUDE_FROM_ALL SYSTEM)
		else()
			add_subdirectory("${p_dir}" "${build_dir}" EXCLUDE_FROM_ALL)
		endif()
	else()
		if(p_system)
			add_subdirectory("${p_dir}" "${build_dir}" SYSTEM)
		else()
			add_subdirectory("${p_dir}" "${build_dir}")
		endif()
	endif()
	set(CMAKE_MESSAGE_LOG_LEVEL "${mng_saved_verbose}")
endfunction()

function(mng_clear_cache)
	cmake_parse_arguments(mng_clear "" "NAME" "" ${ARGN})

	set(cmd_args "--cache-dir" "${mng_source_cache}" "--clear-cache" "--name" "dummy")
	if(mng_clear_NAME)
		list(APPEND cmd_args "--clear-package" "${mng_clear_NAME}")
	endif()

	_mng_run(${cmd_args})

	if(mng_clear_NAME)
		unset(${mng_clear_NAME}_SOURCE_DIR CACHE)
		unset(${mng_clear_NAME}_KEEP_UPDATED CACHE)
	else()
		file(GLOB cache_entries "${CMAKE_BINARY_DIR}/CMakeCache.txt")
		if(cache_entries)
			file(STRINGS "${CMAKE_BINARY_DIR}/CMakeCache.txt" cache_lines)
			foreach(line ${cache_lines})
				if(line MATCHES "^([^:]+)_SOURCE_DIR:")
					string(REGEX REPLACE "^([^:]+)_SOURCE_DIR:.*" "\\1" pkg_name "${line}")
					unset(${pkg_name}_SOURCE_DIR CACHE)
					unset(${pkg_name}_KEEP_UPDATED CACHE)
				endif()
			endforeach()
		endif()
	endif()
endfunction()

function(mng_add_package)
	set(options EXCLUDE_FROM_ALL SYSTEM)
	set(one_value_args NAME VERSION GIT_TAG GITHUB_REPOSITORY GIT_REPOSITORY URL DOWNLOAD_ONLY SUBDIRECTORY VERBOSE)
	set(multi_value_args OPTIONS CMAKE_ARGS)
	cmake_parse_arguments(mng "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

	if(NOT mng_NAME)
		if(mng_GITHUB_REPOSITORY)
			string(REGEX REPLACE ".*/" "" mng_NAME "${mng_GITHUB_REPOSITORY}")
		elseif(mng_GIT_REPOSITORY)
			string(REGEX REPLACE ".*/(.+)\\.git" "\\1" mng_NAME "${mng_GIT_REPOSITORY}")
		elseif(mng_URL)
			string(REGEX REPLACE ".*/(.+)\\.(tar\\.gz|tar\\.bz2|tar\\.xz|zip)" "\\1" mng_NAME "${mng_URL}")
		else()
			message(FATAL_ERROR "MNG: Unable to determine package name")
		endif()
	endif()

	set(cmd_args "--cache-dir" "${mng_source_cache}" "--name" "${mng_NAME}")

	if(mng_VERSION)
		list(APPEND cmd_args "--version" "${mng_VERSION}")
	endif()
	if(mng_GIT_TAG)
		list(APPEND cmd_args "--git-tag" "${mng_GIT_TAG}")
	endif()
	if(mng_GITHUB_REPOSITORY)
		list(APPEND cmd_args "--github-repository" "${mng_GITHUB_REPOSITORY}")
	endif()
	if(mng_GIT_REPOSITORY)
		list(APPEND cmd_args "--git-repository" "${mng_GIT_REPOSITORY}")
	endif()
	if(mng_URL)
		list(APPEND cmd_args "--url" "${mng_URL}")
	endif()

	option(${mng_NAME}_KEEP_UPDATED "Keep ${mng_NAME} updated" OFF)
	if(${mng_NAME}_KEEP_UPDATED)
		list(APPEND cmd_args "--keep-updated")
	endif()

	if(mng_OPTIONS)
		list(APPEND cmd_args "--options" ${mng_OPTIONS})
	endif()

	_mng_run(${cmd_args})

	set(package_dir "${mng_source_cache}/${mng_NAME}/${mng_NAME}")

	if(mng_CMAKE_ARGS)
		foreach(arg ${mng_CMAKE_ARGS})
			set(${arg} CACHE STRING "" FORCE)
		endforeach()
	endif()

	if(mng_OPTIONS)
		foreach(opt ${mng_OPTIONS})
			SET(VALUE_NAME "")
			string(REPLACE " " ";" VALUE_LIST "${opt}")
			list(GET VALUE_LIST 0 VALUE_NAME)
			list(GET VALUE_LIST 1 VALUE_VALUE)

			if(VALUE_VALUE STREQUAL "ON" OR VALUE_VALUE STREQUAL "OFF")
				SET(${VALUE_NAME} ${VALUE_VALUE} CACHE BOOL "" FORCE)
			else()
				SET(${VALUE_NAME} ${VALUE_VALUE} CACHE STRING "" FORCE)
			endif()
		endforeach()
	endif()

	if(NOT mng_DOWNLOAD_ONLY)
		if(mng_SUBDIRECTORY)
			set(source_dir "${package_dir}/${mng_SUBDIRECTORY}")
		else()
			set(source_dir "${package_dir}")
		endif()

		_mng_add_subdirectory(${mng_NAME} "${source_dir}" ${mng_EXCLUDE_FROM_ALL} ${mng_SYSTEM} "${mng_VERBOSE}")
		SET(${mng_NAME}_SOURCE_DIR "${source_dir}" CACHE PATH "" FORCE)
	else()
		set(${mng_NAME}_SOURCE_DIR "${package_dir}" CACHE PATH "" FORCE)
	endif()
endfunction()
