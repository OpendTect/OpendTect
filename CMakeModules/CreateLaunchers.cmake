# - Create launchers to set working directory, env. vars, etc.
#
#  include(CreateLaunchers) - to make these available
#  guess_runtime_library_dirs(<outputvarname> [<extralibrary> ...])
#  create_default_target_launcher(<targetname>
#    [COMMAND <target command>]
#    [ARGS <args...>]
#    [FORWARD_ARGS]
#    [RUNTIME_LIBRARY_DIRS <dir...>]
#    [WORKING_DIRECTORY <dir>]
#    [ENVIRONMENT <VAR=value> [<VAR=value>...]])
#
#  create_target_launcher(<targetname>
#    [COMMAND <target command>]
#    [ARGS <args...>]
#    [FORWARD_ARGS]
#    [RUNTIME_LIBRARY_DIRS <dir...>]
#    [WORKING_DIRECTORY <dir>]
#    [ENVIRONMENT <VAR=value> [<VAR=value>...]])
#
#  create_generic_launcher(<launchername>
#    [COMMAND <target command>]
#    [RUNTIME_LIBRARY_DIRS <dir...>]
#    [WORKING_DIRECTORY <dir>]
#    [ENVIRONMENT <VAR=value> [<VAR=value>...]])
#    - sets GENERIC_LAUNCHER_COMMAND and GENERIC_LAUNCHER_FAIL_REGULAR_EXPRESSION
#
# Requires these CMake modules:
#  CleanDirectoryList
#
# Requires CMake 2.6 or newer (uses the 'function' command)
#
# Original Author:
# 2009-2020 Ryan Pavlik <ryan.pavlik@gmail.edu> <abiryan@ryand.net>
# http://academic.cleardefinition.com
#
# Copyright 2009-2013, Iowa State University.
# Copyright 2018-2019, caseymcc
# Copyright 2020, Ryan Pavlik
# Copyright 2014-2019, Contributors
# SPDX-License-Identifier: BSL-1.0
# Distributed under the Boost Software License, Version 1.0.
# (See accompanying file LICENSE_1_0.txt or copy at
# http://www.boost.org/LICENSE_1_0.txt)

if(__create_launchers)
    return()
endif()
set(__create_launchers YES)

if(POLICY CMP0053)
    cmake_policy(SET CMP0053 NEW)
endif()

include( "${OpendTect_DIR}/CMakeModules/CleanDirectoryList.cmake" )

# We must run the following at "include" time, not at function call time,
# to find the path to this module rather than the path to a calling list file
get_filename_component(_launchermoddir ${CMAKE_CURRENT_LIST_FILE} PATH)
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
        set(_pathdelim "$<SEMICOLON>")
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
    set(_val_args ARGS COMMAND RUNTIME_LIBRARY_DIRS WORKING_DIRECTORY
                  ENVIRONMENT TARGET_PLATFORM)
    set(_bool_args FORWARD_ARGS)
    foreach(_arg ${_val_args} ${_bool_args})
        set(${_arg})
    endforeach()
    foreach(_element ${ARGN})
        string(REPLACE ";" "\\;" _element "${_element}")
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
        message(
            FATAL_ERROR "Syntax error in use of a function in CreateLaunchers!")
    endif()

    # Turn into a list of native paths
    set(_runtime_lib_dirs)
    foreach(_dlldir ${RUNTIME_LIBRARY_DIRS})
        file(TO_NATIVE_PATH "${_dlldir}" _path)
        if(NOT EXISTS "${_path}") #this is not a file so lets leave it as is
            set(_path ${_dlldir})
        endif()
	if ( _runtime_lib_dirs )
	    set( _runtime_lib_dirs "${_runtime_lib_dirs}${_pathdelim}${_path}" )
	else()
	    set( _runtime_lib_dirs "${_path}" )
	endif()
    endforeach()
    if ( _runtime_lib_dirs STREQUAL "" AND
	 CMAKE_VERSION GREATER_EQUAL 3.27 )
	set( _runtime_lib_dirs "$<LIST:REMOVE_DUPLICATES,${_runtime_lib_dirs}>" )
    endif()

    if(NOT WORKING_DIRECTORY)
        set(WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()

    if(NOT COMMAND)
        set(COMMAND "$<TARGET_FILE:${_targetname}>")
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

    if(TARGET_PLATFORM)
        set(USERFILE_PLATFORM ${TARGET_PLATFORM})
    endif()

    set(USERFILE_WORKING_DIRECTORY "${WORKING_DIRECTORY}")
    set(USERFILE_COMMAND "${COMMAND}")
    string(REPLACE ";" " " USERFILE_COMMAND_ARGUMENTS "${ARGS}" )
    string(REPLACE ";" " " LAUNCHERSCRIPT_COMMAND_ARGUMENTS "${ARGS} ${FWD_ARGS}")
    if ( DEFINED ENV{DTECT_BINDIR} AND NOT "${USERFILE_COMMAND}" STREQUAL "" )
	set( USERFILE_COMMAND
	    "$ENV{DTECT_BINDIR}/${_targetname}${OD_EXECUTABLE_EXTENSION}" )
    endif()

    if(WIN32)
	if( _runtime_lib_dirs )
            set(RUNTIME_LIBRARIES_ENVIRONMENT
		"PATH=${_runtime_lib_dirs}${_pathdelim}%PATH%")
        endif()
        file(READ "${_launchermoddir}/launcher.env.cmd.in" _cmdenv)
    else()
        if(_runtime_lib_dirs)
            if(APPLE)

                set(RUNTIME_LIBRARIES_ENVIRONMENT
                    "DYLD_LIBRARY_PATH=${_runtime_lib_dirs}:$DYLD_LIBRARY_PATH")
            else()
                set(RUNTIME_LIBRARIES_ENVIRONMENT
                    "LD_LIBRARY_PATH=${_runtime_lib_dirs}:$LD_LIBRARY_PATH")
            endif()
        endif()
        file(READ "${_launchermoddir}/launcher.env.sh.in" _cmdenv)
    endif()

    set(USERFILE_ENVIRONMENT)
    set(USERFILE_ENV_COMMANDS)
    foreach(_arg "${RUNTIME_LIBRARIES_ENVIRONMENT}" ${ENVIRONMENT})
        if(_arg)
	    if ( "${USERFILE_ENVIRONMENT}" STREQUAL "" )
		set( USERFILE_ENVIRONMENT "${LAUNCHER_LINESEP}" )
	    endif()
	    string(CONFIGURE "@USERFILE_ENVIRONMENT@@_arg@@LAUNCHER_LINESEP@"
				USERFILE_ENVIRONMENT @ONLY)
        endif()
        string(CONFIGURE "@USERFILE_ENV_COMMANDS@${_cmdenv}"
                         USERFILE_ENV_COMMANDS @ONLY)
    endforeach()
    if ( NOT "${USERFILE_ENVIRONMENT}" STREQUAL "" )
	set( USERFILE_ENVIRONMENT "${USERFILE_ENVIRONMENT}    " )
    endif()
    set(USERFILE_ENV_COMMANDS_ORIG ${USERFILE_ENV_COMMANDS})
endmacro()

#ok, we have gone very hackish on this function as the file(GENERATE) is very hard to work with as the cmake folks really don't
#want you writing generator files yourself, yet the user file is not planning to ever be supported
macro(_launcher_produce_vcproj_user)
    if(MSVC)
        set(CMAKEFILES_PATH "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles")
        set(TARGET_CMAKE_FILES "${CMAKEFILES_PATH}/${_targetname}")
        file(READ "${_launchermoddir}/perconfig.${VCPROJ_TYPE}.user.in"
             _perconfig)

        #generator expressions do not play well with ">"
        string(REPLACE ">" "$<ANGLE-R>" _perconfig ${_perconfig})

        set(config_types)
        if(CMAKE_CONFIGURATION_TYPES)
            foreach(config_type ${CMAKE_CONFIGURATION_TYPES})
		list(APPEND config_types ${config_type})
            endforeach()
        else()
            set(config_types ${CMAKE_BUILD_TYPE})
        endif()

        set(USERFILE_CONFIGSECTIONS)
        foreach(USERFILE_CONFIGNAME ${config_types})
            string(CONFIGURE "${_perconfig}" _temp @ONLY ESCAPE_QUOTES)

            #we are building the per config info with generator expressions so that when generating the files only the config currently generated is being correctly filled in
            set(_temp
                "$<IF:$<CONFIG:${USERFILE_CONFIGNAME}>,\n${_temp},$<TARGET_PROPERTY:${_targetname},LAUNCHER_USER_ELSE_${USERFILE_CONFIGNAME}>>\n"
            )

            #we are putting this info into the target properties so it can be recursively added back during the generator calls
            set_target_properties(
                ${_targetname}
                PROPERTIES LAUNCHER_USER_ELSE_${USERFILE_CONFIGNAME} ${_temp})

            string(CONFIGURE "${USERFILE_CONFIGSECTIONS}${_temp}"
                             USERFILE_CONFIGSECTIONS ESCAPE_QUOTES)
        endforeach()

        configure_file(
            "${_launchermoddir}/${VCPROJ_TYPE}.user.in"
            ${TARGET_CMAKE_FILES}.${VCPROJ_TYPE}.${USERFILE_EXTENSION}.config
            @ONLY)

	#now we are looping through each config type loading the previous ones output, hopefully execution order stays the same as the generation request
        set(launcher_last_config)
        foreach(USERFILE_CONFIGNAME ${config_types})
            if(NOT launcher_last_config)
                file(
                    GENERATE
                    OUTPUT
                        ${TARGET_CMAKE_FILES}.${VCPROJ_TYPE}.${USERFILE_CONFIGNAME}.usergen
                    INPUT
                        ${TARGET_CMAKE_FILES}.${VCPROJ_TYPE}.${USERFILE_EXTENSION}.config
                    CONDITION $<CONFIG:${USERFILE_CONFIGNAME}>)
            else()
                file(
                    GENERATE
                    OUTPUT
                        ${TARGET_CMAKE_FILES}.${VCPROJ_TYPE}.${USERFILE_CONFIGNAME}.usergen
                    INPUT
                        ${TARGET_CMAKE_FILES}.${VCPROJ_TYPE}.${launcher_last_config}.usergen
                    CONDITION $<CONFIG:${USERFILE_CONFIGNAME}>)
            endif()
            set(launcher_last_config ${USERFILE_CONFIGNAME})
        endforeach()
        #build the final output from the last generation output
        file(
            GENERATE
            OUTPUT ${VCPROJNAME}.${VCPROJ_TYPE}.${USERFILE_EXTENSION}
            INPUT
                ${TARGET_CMAKE_FILES}.${VCPROJ_TYPE}.${launcher_last_config}.usergen
        )
    endif()

endmacro()

macro(_launcher_configure_executable _src _tmp _target _config)
    if ( DEFINED ENV{DTECT_BINDIR} AND NOT "${USERFILE_COMMAND}" STREQUAL "" )
	if ( WIN32 )
	    set(RUNTIME_PATH_DTECT
			"PATH=$ENV{DTECT_BINDIR}/${OD_EXEC_OUTPUT_RELPATH}${_pathdelim}%PATH%")
	else()
	    set(RUNTIME_PATH_DTECT
			"export PATH=$ENV{DTECT_BINDIR}/${OD_EXEC_OUTPUT_RELPATH}${_pathdelim}\${PATH}")
	endif()
	string(CONFIGURE "@USERFILE_ENV_COMMANDS_ORIG@${RUNTIME_PATH_DTECT}"
		     USERFILE_ENV_COMMANDS @ONLY)
    endif()
    configure_file("${_src}" "${_tmp}" @ONLY)

    #used to convert generator expressions
    file(
        GENERATE
        OUTPUT "${_target}"
        INPUT "${_tmp}"
        CONDITION $<CONFIG:${_config}>)
    #we lose the ability to change the file permissions as there is no support there in file(GENERATE) (although it has been requested)
    #and nothing runs after file(GENERATE) during the cmake call
    #	file(COPY "${_tmp}"
    #	    DESTINATION "${_targetpath}"
    #	    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endmacro()

macro(_launcher_create_target_launcher)
    if(CMAKE_CONFIGURATION_TYPES)
        # Multi-config generator - multiple launchers
        foreach(_config ${CMAKE_CONFIGURATION_TYPES})
            set(_fn "launch-${_targetname}-${_config}.${_suffix}")
            _launcher_configure_executable(
                "${_launchermoddir}/targetlauncher.${_suffix}.in"
                "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${_fn}"
                "${CMAKE_CURRENT_BINARY_DIR}/${_fn}" ${_config})
        endforeach()
    else()
        # Single-config generator - single launcher
        _launcher_configure_executable(
            "${_launchermoddir}/targetlauncher.${_suffix}.in"
            "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/launch-${_targetname}.${_suffix}"
            "${CMAKE_CURRENT_BINARY_DIR}/launch-${_targetname}.${_suffix}"
            ${CMAKE_BUILD_TYPE})
    endif()
endmacro()

function(create_default_target_launcher _targetname)
    _launcher_system_settings()
    _launcher_process_args(${ARGN})

    set(VCPROJNAME "${CMAKE_BINARY_DIR}/ALL_BUILD")

    _launcher_produce_vcproj_user()
    _launcher_create_target_launcher()
endfunction()

function(create_target_launcher _targetname)
    _launcher_system_settings()
    _launcher_process_args(${ARGN})

    set(VCPROJNAME "${CMAKE_CURRENT_BINARY_DIR}/${_targetname}")

    _launcher_produce_vcproj_user()
    _launcher_create_target_launcher()
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
        set(GENERIC_LAUNCHER_COMMAND
            "${_launchername}"
            PARENT_SCOPE)
        set(GENERIC_LAUNCHER_FAIL_REGULAR_EXPRESSION)
    else()
        set(GENERIC_LAUNCHER_COMMAND
            sh "${_launchername}"
            PARENT_SCOPE)
        set(GENERIC_LAUNCHER_FAIL_REGULAR_EXPRESSION
            "Program terminated with signal")
    endif()

    if(CMAKE_CONFIGURATION_TYPES)
        set(_launcher_build_type "${CMAKE_BUILD_TYPE}")
    else()
        unset(_launcher_build_type)
    endif()

    _launcher_configure_executable(
        "${_launchermoddir}/genericlauncher.${_suffix}.in"
        "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/genericlauncher.${_suffix}.in"
        "${_launchername}" ${_launcher_build_type})
endfunction()

