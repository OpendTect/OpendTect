# - Create launchers to set working directory, env. vars, etc.
#
#  include(CreateLaunchers) - to make these available
#  guess_runtime_library_dirs(<outputvarname> [<extralibrary> ...])
#  create_default_target_launcher(<targetname>
#    [ARGS <args...>]
#    [FORWARD_ARGS]
#    [RUNTIME_LIBRARY_DIRS <dir...>]
#    [WORKING_DIRECTORY <dir>]
#    [ENVIRONMENT <VAR=value> [<VAR=value>...]])
#
#  create_target_launcher(<targetname>
#    [ARGS <args...>]
#    [FORWARD_ARGS]
#    [RUNTIME_LIBRARY_DIRS <dir...>]
#    [WORKING_DIRECTORY <dir>]
#    [ENVIRONMENT <VAR=value> [<VAR=value>...]])
#
#  create_generic_launcher(<launchername>
#    [RUNTIME_LIBRARY_DIRS <dir...>]
#    [WORKING_DIRECTORY <dir>]
#    [ENVIRONMENT <VAR=value> [<VAR=value>...]])
#    - sets GENERIC_LAUNCHER_COMMAND and GENERIC_LAUNCHER_FAIL_REGULAR_EXPRESSION
#
# Requires these CMake modules:
#  ListFilter
#  ProgramFilesGlob
#  CleanDirectoryList
#
# Requires CMake 2.6 or newer (uses the 'function' command)
#
# Original Author:
# 2009-2010 Ryan Pavlik <rpavlik@iastate.edu> <abiryan@ryand.net>
# http://academic.cleardefinition.com
# Iowa State University HCI Graduate Program/VRAC
#
# Copyright Iowa State University 2009-2010.
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

if(POLICY CMP0026)
    cmake_policy(SET CMP0026 NEW)
endif()

if(POLICY CMP0053)
    cmake_policy(SET CMP0053 NEW)
endif()


if(__create_launchers)
	return()
endif()
set(__create_launchers YES)

include(CleanDirectoryList)

# We must run the following at "include" time, not at function call time,
# to find the path to this module rather than the path to a calling list file
get_filename_component(_launchermoddir
	${CMAKE_CURRENT_LIST_FILE}
	PATH)
set(_launchermoddir "${_launchermoddir}/launcher-templates")

macro(_launcher_system_settings)
	if(CMAKE_SIZEOF_VOID_P EQUAL 8)
		set(BITS 64)
	else()
		set(BITS 32)
	endif()

	if(WIN32)
		# Find user and system name
		set(SYSTEM_NAME $ENV{USERDOMAIN})
		set(USER_NAME $ENV{USERNAME})
		if(MSVC_VERSION VERSION_GREATER_EQUAL 1600)
			set(LAUNCHER_LINESEP "\n")
			set(USERFILE_EXTENSION user)
			set(VCPROJ_TYPE vcxproj)
		else()
			set(LAUNCHER_LINESEP "&#x0A;")
			set(USERFILE_EXTENSION ${SYSTEM_NAME}.${USER_NAME}.user)
			set(VCPROJ_TYPE vcproj)
		endif()

		if(MSVC_VERSION VERSION_GREATER_EQUAL 1920)
			set(USERFILE_VC_VERSION 16.00)
		elseif(MSVC_VERSION VERSION_GREATER_EQUAL 1910)
			set(USERFILE_VC_VERSION 15.00)
		elseif(MSVC_VERSION VERSION_GREATER_EQUAL 1900)
			set(USERFILE_VC_VERSION 14.00)
		elseif(MSVC_VERSION VERSION_GREATER_EQUAL 1800)
			set(USERFILE_VC_VERSION 12.00)
		elseif(MSVC_VERSION VERSION_GREATER_EQUAL 1700)
			set(USERFILE_VC_VERSION 11.00)
		elseif(MSVC_VERSION VERSION_GREATER_EQUAL 1600)
			set(USERFILE_VC_VERSION 10.00)
		elseif(MSVC_VERSION VERSION_GREATER_EQUAL 1500)
			set(USERFILE_VC_VERSION 9.00)
		elseif(MSVC_VERSION VERSION_GREATER_EQUAL 1400)
			set(USERFILE_VC_VERSION 8.00)
		elseif(MSVC_VERSION VERSION_GREATER_EQUAL 1310)
			set(USERFILE_VC_VERSION 7.10)
		elseif(MSVC)
			message(STATUS "MSVC but unrecognized version!")
		endif()
		if(BITS EQUAL 64)
			set(USERFILE_PLATFORM x64)
		else()
			set(USERFILE_PLATFORM Win${BITS})
		endif()
		set(_pathdelim ";")
		set(_suffix "cmd")
	else()
		set(_pathdelim ":")
		set(USERFILE_PLATFORM ${CMAKE_SYSTEM_NAME}${BITS})
		set(_suffix "sh")
		find_package(GDB QUIET)
		if(GDB_FOUND)
			set(LAUNCHERS_GOT_GDB YES)
			if(GDB_HAS_RETURN_CHILD_RESULT)
				set(LAUNCHERS_GDB_ARG --return-child-result)
			endif()
		else()
			set(LAUNCHERS_GOT_GDB)
		endif()
	endif()

	if(WIN32 AND NOT USERFILE_REMOTE_MACHINE)
		site_name(USERFILE_REMOTE_MACHINE)
		mark_as_advanced(USERFILE_REMOTE_MACHINE)
	endif()
endmacro()

macro(_launcher_process_args)
	set(_nowhere)
	set(_curdest _nowhere)
	set(_val_args
		ARGS
		RUNTIME_LIBRARY_DIRS
		WORKING_DIRECTORY
		ENVIRONMENT)
	set(_bool_args FORWARD_ARGS)
	foreach(_arg ${_val_args} ${_bool_args})
		set(${_arg})
	endforeach()
	foreach(_element ${ARGN})
		list(FIND _val_args "${_element}" _val_arg_find)
		list(FIND _bool_args "${_element}" _bool_arg_find)
		if("${_val_arg_find}" GREATER "-1")
			set(_curdest "${_element}")
		elseif("${_bool_arg_find}" GREATER "-1")
			set("${_element}" ON)
			set(_curdest _nowhere)
		else()
			list(APPEND ${_curdest} "${_element}")
		endif()
	endforeach()

	if(_nowhere)
		message(FATAL_ERROR
			"Syntax error in use of a function in CreateLaunchers!")
	endif()

	# Turn into a list of native paths
	set(_runtime_lib_dirs)
	foreach(_dlldir ${RUNTIME_LIBRARY_DIRS})
		file(TO_NATIVE_PATH "${_dlldir}" _path)
		set(_runtime_lib_dirs "${_runtime_lib_dirs}${_path}${_pathdelim}")
	endforeach()

	if(NOT WORKING_DIRECTORY)
		set(WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
	endif()

	if(FORWARD_ARGS)
		if(WIN32)
			set(FWD_ARGS %*)
		else()
			set(FWD_ARGS $*)
		endif()
	else()
		set(FWD_ARGS)
	endif()

	set(USERFILE_WORKING_DIRECTORY "${WORKING_DIRECTORY}")
	set(USERFILE_COMMAND_ARGUMENTS "${ARGS}")
	set(LAUNCHERSCRIPT_COMMAND_ARGUMENTS "${ARGS} ${FWD_ARGS}")

	if(WIN32)
		set(RUNTIME_LIBRARIES_ENVIRONMENT "PATH=${_runtime_lib_dirs};%PATH%")
		file(READ
			"${_launchermoddir}/launcher.env.cmd.in"
			_cmdenv)
	else()
		if(APPLE)
			set(RUNTIME_LIBRARIES_ENVIRONMENT
				"DYLD_LIBRARY_PATH=${_runtime_lib_dirs}:$DYLD_LIBRARY_PATH")
		else()
			set(RUNTIME_LIBRARIES_ENVIRONMENT
				"LD_LIBRARY_PATH=${_runtime_lib_dirs}:$LD_LIBRARY_PATH")
		endif()
		file(READ
			"${_launchermoddir}/launcher.env.sh.in"
			_cmdenv)
	endif()
	set(USERFILE_ENVIRONMENT "${RUNTIME_LIBRARIES_ENVIRONMENT}")

	set(USERFILE_ENV_COMMANDS)
	foreach(_arg "${RUNTIME_LIBRARIES_ENVIRONMENT}" ${ENVIRONMENT})
		string(CONFIGURE
			"@USERFILE_ENVIRONMENT@@LAUNCHER_LINESEP@@_arg@"
			USERFILE_ENVIRONMENT
			@ONLY)
		string(CONFIGURE
			"@USERFILE_ENV_COMMANDS@${_cmdenv}"
			USERFILE_ENV_COMMANDS
			@ONLY)
	endforeach()
endmacro()

macro(_launcher_produce_vcproj_user)
	if(MSVC)
		file(READ
			"${_launchermoddir}/perconfig.${VCPROJ_TYPE}.user.in"
			_perconfig)
		set(USERFILE_CONFIGSECTIONS)
		foreach(USERFILE_CONFIGNAME ${CMAKE_CONFIGURATION_TYPES})
			set(_temp
                "$<IF:$<CONFIG:${USERFILE_CONFIGNAME}>,\n${_temp},$<TARGET_PROPERTY:${_targetname},LAUNCHER_USER_ELSE_${USERFILE_CONFIGNAME}>>\n"
            )
			file(TO_NATIVE_PATH "${_temp}" _temp)
			string(CONFIGURE "${_perconfig}" _temp @ONLY ESCAPE_QUOTES)
			string(CONFIGURE
				"${USERFILE_CONFIGSECTIONS}${_temp}"
				USERFILE_CONFIGSECTIONS
				ESCAPE_QUOTES)
		endforeach()

		configure_file("${_launchermoddir}/${VCPROJ_TYPE}.user.in"
			${VCPROJNAME}.${VCPROJ_TYPE}.${USERFILE_EXTENSION}
			@ONLY)
	endif()

endmacro()

macro(_launcher_create_target_launcher)
	if(CMAKE_CONFIGURATION_TYPES)
		# Multi-config generator - multiple launchers
		foreach(_config ${CMAKE_CONFIGURATION_TYPES})
			get_target_property(USERFILE_${_config}_COMMAND
				${_targetname}
				LOCATION_${_config})
			file(TO_NATIVE_PATH
				"${USERFILE_${_config}_COMMAND}"
				USERFILE_COMMAND)
			configure_file("${_launchermoddir}/targetlauncher.${_suffix}.in"
				"${CMAKE_CURRENT_BINARY_DIR}/launch-${_targetname}-${_config}.${_suffix}"
				@ONLY)
		endforeach()
	else()
		# Single-config generator - single launcher
		get_target_property(USERFILE_COMMAND
			${_targetname}
			LOCATION)
		file(TO_NATIVE_PATH
			"${USERFILE_COMMAND}"
			USERFILE_COMMAND)
		configure_file("${_launchermoddir}/targetlauncher.${_suffix}.in"
			"${CMAKE_CURRENT_BINARY_DIR}/launch-${_targetname}.${_suffix}"
			@ONLY)
	endif()
endmacro()

function(create_default_target_launcher _targetname)
	_launcher_system_settings()
	_launcher_process_args(${ARGN})

	set(VCPROJNAME "${CMAKE_BINARY_DIR}/ALL_BUILD")
	_launcher_produce_vcproj_user()
endfunction()

function(create_target_launcher _targetname)
	_launcher_system_settings()
	_launcher_process_args(${ARGN})

	set(VCPROJNAME "${CMAKE_CURRENT_BINARY_DIR}/${_targetname}")
	_launcher_produce_vcproj_user()

	if ( NOT WIN32 )
		_launcher_create_target_launcher()
	endif()
endfunction()

function(create_generic_launcher _launchername)
	_launcher_system_settings()
	_launcher_process_args(${ARGN})

	if(NOT IS_ABSOLUTE _launchername)
		set(_launchername
			"${CMAKE_CURRENT_BINARY_DIR}/${_launchername}.${_suffix}")
	else()
		set(_launchername "${_launchername}.${_suffix}")
	endif()
	if(WIN32)
		set(GENERIC_LAUNCHER_COMMAND "${_launchername}" PARENT_SCOPE)
		set(GENERIC_LAUNCHER_FAIL_REGULAR_EXPRESSION)
	else()
		set(GENERIC_LAUNCHER_COMMAND sh "${_launchername}" PARENT_SCOPE)
		set(GENERIC_LAUNCHER_FAIL_REGULAR_EXPRESSION
			"Program terminated with signal")
	endif()

	configure_file("${_launchermoddir}/genericlauncher.${_suffix}.in"
		"${_launchername}"
		@ONLY)
endfunction()
