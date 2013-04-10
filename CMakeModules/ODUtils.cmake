#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id$
#_______________________________________________________________________________

if ( (CMAKE_GENERATOR STREQUAL "Unix Makefiles") OR
     (CMAKE_GENERATOR STREQUAL "Ninja") )
    if ( CMAKE_BUILD_TYPE STREQUAL "" )
	set ( DEBUGENV $ENV{DEBUG} )
	if ( DEBUGENV AND
	    ( (${DEBUGENV} MATCHES "yes" ) OR
	      (${DEBUGENV} MATCHES "Yes" ) OR
	      (${DEBUGENV} MATCHES "YES" ) ) )
	    set ( CMAKE_BUILD_TYPE "Debug"
		  CACHE STRING "Debug or Release" FORCE )
	else()
	    set ( CMAKE_BUILD_TYPE "Release"
		  CACHE STRING "Debug or Release" FORCE)
	endif()

	message( STATUS "Setting CMAKE_BUILD_TYPE to ${CMAKE_BUILD_TYPE}" )
    endif()

    set( OD_BUILDSUBDIR "/${CMAKE_BUILD_TYPE}" )
endif()

set ( OD_TESTDATA_DIR "" CACHE FILEPATH "Test data location" )
configure_file ( ${OpendTect_DIR}/CMakeModules/templates/CTestCustom.ctest.in
		 ${CMAKE_BINARY_DIR}/CTestCustom.ctest )

add_definitions("-D__cmake__")

set ( OD_SOURCELIST_FILE ${CMAKE_BINARY_DIR}/CMakeModules/sourcefiles.txt )
file ( REMOVE ${OD_SOURCELIST_FILE} )

set ( OD_EXEC_OUTPUT_RELPATH "bin/${OD_PLFSUBDIR}${OD_BUILDSUBDIR}" )
set ( OD_EXEC_OUTPUT_PATH "${OD_BINARY_BASEDIR}/${OD_EXEC_OUTPUT_RELPATH}" )
set ( OD_EXEC_INSTALL_PATH "${OD_EXEC_OUTPUT_RELPATH}" )
set ( OD_BUILD_VERSION "${OpendTect_VERSION_MAJOR}.${OpendTect_VERSION_MINOR}.${OpendTect_VERSION_DETAIL}${OpendTect_VERSION_PATCH}")
set ( OD_API_VERSION "${OpendTect_VERSION_MAJOR}.${OpendTect_VERSION_MINOR}.${OpendTect_VERSION_DETAIL}" )

set ( OD_MAIN_EXEC od_main )
set ( OD_ATTRIB_EXECS od_process_attrib od_process_attrib_em od_stratamp )
set ( OD_VOLUME_EXECS od_process_volume )
set ( OD_PRESTACK_EXECS od_process_prestack )
set ( OD_ZAXISTRANSFORM_EXECS od_process_time2depth )

#Macro for going through a list of modules and adding them
macro ( OD_ADD_MODULES )
    set( DIR ${ARGV0} )

    foreach( OD_MODULE_NAME ${ARGV} )
	if ( NOT ${OD_MODULE_NAME} STREQUAL ${DIR} )
	    add_subdirectory( ${DIR}/${OD_MODULE_NAME} )
	endif()
    endforeach()
endmacro()

# Macro for going through a list of modules and adding them
# as optional targets
macro ( OD_ADD_OPTIONAL_MODULES )
    set( DIR ${ARGV0} )

    foreach( OD_MODULE_NAME ${ARGV} )
	if ( NOT ${OD_MODULE_NAME} MATCHES ${DIR} )
	    add_subdirectory( ${DIR}/${OD_MODULE_NAME} EXCLUDE_FROM_ALL )
	endif()
    endforeach()
endmacro()

macro ( OD_INSTALL_LIBRARY SOURCE )
  get_filename_component( PATH ${SOURCE} REALPATH )
  get_filename_component( FILENAME ${SOURCE} NAME )
  install( PROGRAMS ${PATH} DESTINATION ${OD_EXEC_OUTPUT_RELPATH} RENAME ${FILENAME} )
endmacro()
