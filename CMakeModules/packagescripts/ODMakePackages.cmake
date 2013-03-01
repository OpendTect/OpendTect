#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to build a release
# Author:       K. Tingdahl
# Date:		August 2012		
#RCS:           $Id$

set( BASEPACKAGES basedatadefs dgbbasedatadefs)
set( PACKAGELIST basedefs dgbbasedefs dgbccbdefs dgbdsdefs dgbhcdefs
		 dgbnndefs dgbssisdefs dgbstratdefs dgbvmbdefs dgbwcpdefs
		 odgmtdefs odgprdefs odmadagascardefs develdefs ) 

include( CMakeModules/packagescripts/ODMakePackagesUtils.cmake )

if( APPLE )
    execute_process( COMMAND ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/${CMAKE_BUILD_TYPE}/chfwscript ${PSD}/bin/${OD_PLFSUBDIR}/${CMAKE_BUILD_TYPE} ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/${CMAKE_BUILD_TYPE} 
		     RESULT_VARIABLE STATUS )
    if( NOT ${STATUS} EQUAL "0" )
	message( FATAL_ERROR "changing dependency Failed" )
    endif()
endif()

if( APPLE OR WIN32 )
    od_sign_libs()
endif()

foreach ( BASEPACKAGE ${BASEPACKAGES} )
    include( ${PSD}/CMakeModules/packagescripts/${BASEPACKAGE}.cmake)
    init_destinationdir( ${PACK} )
    create_basepackages( ${PACK} )
endforeach()

foreach ( PACKAGE ${PACKAGELIST} )
    include(CMakeModules/packagescripts/${PACKAGE}.cmake)
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
	if( NOT EXISTS "${PSD}/bin/win/zip.exe" )
	    message( FATAL_ERROR "${PSD}/bin/win/zip.exe is not existed.
		     Unable to create packages.Please do an update" )
	endif()
    endif()

    init_destinationdir( ${PACK} )
    if( ${PACK} STREQUAL "devel" )
        create_develpackages()
    else()
	create_package( ${PACK} )
    endif()
endforeach()
message( "\n Created packages are available under ${PSD}/packages" )
