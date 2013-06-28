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

add_definitions("-D__cmake__")

set ( OD_SOURCELIST_FILE ${CMAKE_BINARY_DIR}/CMakeModules/sourcefiles.txt )
file ( REMOVE ${OD_SOURCELIST_FILE} )

set ( OD_EXEC_OUTPUT_RELPATH "bin/${OD_PLFSUBDIR}${OD_BUILDSUBDIR}" )
set ( OD_EXEC_OUTPUT_PATH "${CMAKE_BINARY_DIR}/${OD_EXEC_OUTPUT_RELPATH}" )
if ( WIN32 )
    set ( OD_EXEC_INSTALL_PATH_RELEASE "${OD_EXEC_OUTPUT_RELPATH}/Release" )
    set ( OD_EXEC_INSTALL_PATH_DEBUG "${OD_EXEC_OUTPUT_RELPATH}/Debug" )
    set ( CMAKE_PDB_OUTPUT_DIRECTORY "${OD_EXEC_OUTPUT_RELPATH}" )
else()
    set ( OD_EXEC_INSTALL_PATH_RELEASE "bin/${OD_PLFSUBDIR}/Release" )
    set ( OD_EXEC_INSTALL_PATH_DEBUG "bin/${OD_PLFSUBDIR}/Debug" )
endif()
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
    list( APPEND OD_MODULE_THIRD_PARTY_LIBS ${OD_THIRD_PARTY_LIBS} ${FILENAME} )
    list( REMOVE_DUPLICATES OD_MODULE_THIRD_PARTY_LIBS )
    set ( OD_THIRD_PARTY_LIBS ${OD_MODULE_THIRD_PARTY_LIBS} PARENT_SCOPE ) 
    install( PROGRAMS ${PATH} DESTINATION ${OD_EXEC_OUTPUT_RELPATH} RENAME ${FILENAME} )
endmacro()


#Takes a library variable with both _RELEASE and _DEBUG variants, and constructs
# a variable that combinse both

macro ( OD_MERGE_LIBVAR VARNAME )
    if ( ${${VARNAME}_RELEASE} STREQUAL "${VARNAME}_RELEASE-NOTFOUND" )
	set( RELNOTFOUND 1 )
    endif()
    if ( ${${VARNAME}_DEBUG} STREQUAL "${VARNAME}_DEBUG-NOTFOUND" )
	set( DEBNOTFOUND 1 )
    endif()

    if ( DEFINED RELNOTFOUND AND DEFINED DEBNOTFOUND )
	message( FATAL_ERROR "${VARNAME} not found" )
    endif()

    if ( (NOT DEFINED RELNOTFOUND) )
	if ( NOT DEFINED DEBNOTFOUND )
	    set ( ${VARNAME} "optimized" ${${VARNAME}_RELEASE} "debug" ${${VARNAME}_DEBUG}  )
	else()
	    set ( ${VARNAME} ${${VARNAME}_RELEASE} )
	endif()
    else()
	set ( ${VARNAME} ${${VARNAME}_DEBUG} )
    endif()

    unset( RELNOTFOUND )
    unset( DEBNOTFOUND )
endmacro()


#Takes a list with both optimized and debug libraries, and removes one of them
#According to BUILD_TYPE

macro ( OD_FILTER_LIBRARIES INPUTLIST BUILD_TYPE )
    unset( OUTPUT )
    foreach ( LISTITEM ${${INPUTLIST}} )
	if ( DEFINED USENEXT )
	    if ( USENEXT STREQUAL "yes" )
		list ( APPEND OUTPUT ${LISTITEM} )
	    endif()
	    unset( USENEXT )
	else()
	    if ( LISTITEM STREQUAL "debug" )
		if ( "${BUILD_TYPE}" STREQUAL "Debug" )
		    set ( USENEXT "yes" )
		else()
		    set ( USENEXT "no" )
		endif()
	    else()
		if ( LISTITEM STREQUAL "optimized" )
		    if ( "${BUILD_TYPE}" STREQUAL "Release" )
			set ( USENEXT "yes" )
		    else()
			set ( USENEXT "no" )
		    endif()
		else()
		    list ( APPEND OUTPUT ${LISTITEM} )
		endif()
	    endif()
	endif()
    endforeach()

    set ( ${INPUTLIST} ${OUTPUT} )
endmacro()


