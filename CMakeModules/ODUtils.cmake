#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id$
#_______________________________________________________________________________

if ( CMAKE_GENERATOR STREQUAL "Unix Makefiles" )
    if ( CMAKE_BUILD_TYPE STREQUAL "" )
	set ( DEBUGENV $ENV{DEBUG} )
	if ( DEBUGENV AND
	    ( (${DEBUGENV} MATCHES "yes" ) OR
	      (${DEBUGENV} MATCHES "Yes" ) OR
	      (${DEBUGENV} MATCHES "YES" ) ) )
	    set ( CMAKE_BUILD_TYPE "Debug" CACHE STRING "Debug or Release" FORCE )
	else()
	    set ( CMAKE_BUILD_TYPE "Release" CACHE STRING "Debug or Release" FORCE)
	endif()

	MESSAGE( STATUS "Setting CMAKE_BUILD_TYPE to ${CMAKE_BUILD_TYPE}" )
    endif()

    set( OD_BUILDSUBDIR "/${CMAKE_BUILD_TYPE}" )
endif()

ADD_DEFINITIONS("-D__cmake__")

set ( OD_EXEC_OUTPUT_RELPATH "bin/${OD_PLFSUBDIR}${OD_BUILDSUBDIR}" )
set ( OD_EXEC_OUTPUT_PATH "${OD_BINARY_BASEDIR}/${OD_EXEC_OUTPUT_RELPATH}" )
set ( OD_EXEC_INSTALL_PATH "${OD_EXEC_OUTPUT_RELPATH}" )

set ( OD_MAIN_EXEC od_main )
set ( OD_ATTRIB_EXECS od_process_attrib od_process_attrib_em )
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
