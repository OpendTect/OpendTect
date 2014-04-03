#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to build a release
# Author:       K. Tingdahl
# Date:		August 2012		
#RCS:           $Id$

message( "SOURCE_DIR: ${SOURCE_DIR}" )
message( "BINARY_DIR: ${BINARY_DIR}" )

include( ${SOURCE_DIR}/CMakeModules/packagescripts/packages.cmake )
include( ${SOURCE_DIR}/CMakeModules/packagescripts/ODMakePackagesUtils.cmake )

#Genarate Symbols and then Strip the binaries
OD_GENERATE_BREAKPAD_SYMBOLS()

if( APPLE OR WIN32 )
    od_sign_libs()
endif()

foreach ( BASEPACKAGE ${BASEPACKAGES} )
	include( ${SOURCE_DIR}/CMakeModules/packagescripts/${BASEPACKAGE}.cmake)
    init_destinationdir( ${PACK} )
    create_basepackages( ${PACK} )
endforeach()

foreach ( PACKAGE ${PACKAGELIST} )
	include(${SOURCE_DIR}/CMakeModules/packagescripts/${PACKAGE}.cmake)
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
    else()
	create_package( ${PACK} )
    endif()
endforeach()
message( "\n Created packages are available under ${SOURCE_DIR}/packages" )
