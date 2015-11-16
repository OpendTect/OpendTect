#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to build a release
# Author:       K. Tingdahl
# Date:		August 2012		
#RCS:           $Id$

message( "SOURCE_DIR: ${SOURCE_DIR}" )
message( "BINARY_DIR: ${BINARY_DIR}" )

if ( EXISTS  CMakeModules/packagescripts/packages.cmake )
	#//to read file on dgb if CMAKE_BUILD_DIR is different from CMAKE_SOURCE_DIR
	include( CMakeModules/packagescripts/packages.cmake ) 
elseif ( EXISTS ${SOURCE_DIR}/CMakeModules/packagescripts/packages.cmake ) 
	#to read file on od if CMAKE_BUILD_DIR is different from CMAKE_SOURCE_DIR
    include( ${SOURCE_DIR}/CMakeModules/packagescripts/packages.cmake ) 
else()
    message( FATAL_ERROR "File packages.cmake not found" )
endif()
include( ${SOURCE_DIR}/CMakeModules/packagescripts/ODMakePackagesUtils.cmake )

if( UNIX )
    foreach ( BASEPACKAGE ${BASEPACKAGES} )
	if ( EXISTS  CMakeModules/packagescripts/${BASEPACKAGE}.cmake )
	   include( CMakeModules/packagescripts/${BASEPACKAGE}.cmake)
	elseif( EXISTS ${SOURCE_DIR}/CMakeModules/packagescripts/${BASEPACKAGE}.cmake )
	   include( ${SOURCE_DIR}/CMakeModules/packagescripts/${BASEPACKAGE}.cmake )
	endif()
	init_destinationdir( ${PACK} )
	create_basepackages( ${PACK} )
    endforeach()
endif()

foreach ( PACKAGE ${PACKAGELIST} )
    if ( ("${PACKAGE}" STREQUAL "doc") OR ("${PACKAGE}" STREQUAL "dgbdoc") OR
       ("${PACKAGE}" STREQUAL "classdoc") )
	set( PACK ${PACKAGE} )
    else()
	if ( EXISTS  CMakeModules/packagescripts/${PACKAGE}.cmake )
	    include( CMakeModules/packagescripts/${PACKAGE}.cmake)
	elseif( EXISTS ${SOURCE_DIR}/CMakeModules/packagescripts/${PACKAGE}.cmake )
	   include( ${SOURCE_DIR}/CMakeModules/packagescripts/${PACKAGE}.cmake )
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

    if( ${OD_PLFSUBDIR} STREQUAL "win32" OR ${OD_PLFSUBDIR} STREQUAL "win64" )
	    if( NOT EXISTS "${SOURCE_DIR}/bin/win64/zip.exe" )
		message( FATAL_ERROR "${SOURCE_DIR}/bin/win64/zip.exe is not existed. Unable to create packages." )
	endif()
    endif()

    init_destinationdir( ${PACK} )
    if( ${PACK} STREQUAL "devel" )
        create_develpackages()
    elseif( ${PACK} STREQUAL "doc" )
	    create_docpackages( doc )
    elseif( ${PACK} STREQUAL "dgbdoc" )
	    create_docpackages( dgbdoc )
    elseif( ${PACK} STREQUAL "classdoc" )
	    create_docpackages( classdoc )
    else()
	if ( "${OD_ENABLE_BREAKPAD}" STREQUAL "ON" )
	    #Genarate Symbols on Win and Linux platofrms. Also strip libs in Linux
	    set( ALLLIBS ${LIBLIST} ${PLUGINS} )
	    OD_GENERATE_BREAKPAD_SYMBOLS( "${ALLLIBS}" "${EXECLIST}" )
	endif()

	create_package( ${PACK} )
    endif()
endforeach()
if ( "${OD_ENABLE_BREAKPAD}" STREQUAL "ON" )
    zippackage( ${SYMBOLDIRNM}.zip ${SYMBOLDIRNM} ${PACKAGE_DIR}/symbols )
endif()
message( "\n Created packages are available under ${PACKAGE_DIR}" )
