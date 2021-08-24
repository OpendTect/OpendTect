#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to build a release
# Author:       K. Tingdahl
# Date:		August 2012		

if ( "${OpendTect_DIR}" STREQUAL "" )
    set( OpendTect_DIR "${SOURCE_DIR}" )
endif()

if ( EXISTS ${SOURCE_DIR}/CMakeModules/packagescripts/packages.cmake ) 
    include( ${SOURCE_DIR}/CMakeModules/packagescripts/packages.cmake ) 
else()
    message( FATAL_ERROR "File packages.cmake not found" )
endif()
include( ${OpendTect_DIR}/CMakeModules/packagescripts/ODMakePackagesUtils.cmake )

if ( "${CMAKE_BUILD_TYPE}" STREQUAL "Release" )
    foreach ( BASEPACKAGE ${BASEPACKAGES} )
	if( EXISTS ${BINARY_DIR}/CMakeModules/packagescripts/${BASEPACKAGE}.cmake )
	   include( ${BINARY_DIR}/CMakeModules/packagescripts/${BASEPACKAGE}.cmake )
	elseif( EXISTS ${SOURCE_DIR}/CMakeModules/packagescripts/${BASEPACKAGE}.cmake )
	   include( ${SOURCE_DIR}/CMakeModules/packagescripts/${BASEPACKAGE}.cmake )
	else()
	    message( FATAL_ERROR "Configuration file not found for package ${BASEPACKAGE}" )
	endif()
	INIT_DESTINATIONDIR( ${PACK} )
	CREATE_BASEPACKAGES( ${PACK} )
    endforeach()
endif()

foreach ( PACKAGE ${PACKAGELIST} )
    if( ("${PACKAGE}" STREQUAL "doc") OR ("${PACKAGE}" STREQUAL "dgbdoc") OR
       ("${PACKAGE}" STREQUAL "classdoc") )
	set( PACK ${PACKAGE} )
    else()
	if( EXISTS ${BINARY_DIR}/CMakeModules/packagescripts/${PACKAGE}.cmake )
	   include( ${BINARY_DIR}/CMakeModules/packagescripts/${PACKAGE}.cmake )
	elseif( EXISTS ${SOURCE_DIR}/CMakeModules/packagescripts/${PACKAGE}.cmake )
	   include( ${SOURCE_DIR}/CMakeModules/packagescripts/${PACKAGE}.cmake )
	else()
	    message( FATAL_ERROR "Configuration file not found for package ${PACKAGE}" )
	endif()
    endif()
    if( NOT DEFINED OpendTect_VERSION_MAJOR )
	message( FATAL_ERROR "OpendTect_VERSION_MAJOR not defined" )
    endif()

    if( NOT DEFINED OD_PLFSUBDIR )
	message( FATAL_ERROR "OD_PLFSUBDIR not defined" )
    endif()

    if( NOT DEFINED CMAKE_INSTALL_PREFIX )
	message( FATAL_ERROR "CMAKE_INSTALL_PREFIX is not Defined. " )
    endif()

    if( WIN32 )
	if( NOT EXISTS "${OpendTect_DIR}/bin/win64/zip.exe" )
	    message( FATAL_ERROR "${OpendTect_DIR}/bin/win64/zip.exe does not exist. Unable to create packages." )
	endif()
    else()
	find_program( ZIP_EXEC "zip" NO_CACHE )
	if ( NOT ZIP_EXEC )
	    message( FATAL_ERROR "'zip' executable is not installed or not in the path. Unable to create packages." )
	endif()
    endif( WIN32 )

    INIT_DESTINATIONDIR( ${PACK} )
    if( ${PACK} STREQUAL "devel" )
	CREATE_DEVELPACKAGES()
    elseif( ${PACK} STREQUAL "classdoc" )
	CREATE_DOCPACKAGES( classdoc )
    endif()

    if( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
	continue()
    endif()

    if( ${PACK} STREQUAL "doc" )
	CREATE_DOCPACKAGES( doc )
    elseif( ${PACK} STREQUAL "dgbdoc" )
	CREATE_DOCPACKAGES( dgbdoc )
    else()
	CREATE_PACKAGE( ${PACK} )
    endif()
endforeach()

if( "${OD_ENABLE_BREAKPAD}" STREQUAL "ON" )
    set( SYMBOLDIRNM symbols_${OD_PLFSUBDIR}_${OpendTect_FULL_VERSION} )
    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
		     ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/${CMAKE_BUILD_TYPE}/symbols
		     ${PACKAGE_DIR}/symbols/${SYMBOLDIRNM} )
    ZIPPACKAGE( ${SYMBOLDIRNM}.zip ${SYMBOLDIRNM} ${PACKAGE_DIR}/symbols )
endif()
message( STATUS "\n Created packages are available under ${PACKAGE_DIR}" )
