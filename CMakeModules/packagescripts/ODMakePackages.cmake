#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to build a release
# Author:       K. Tingdahl
# Date:		August 2012		
#RCS:           $Id: ODMakePackages.cmake,v 1.9 2012/09/11 12:25:44 cvsnageswara Exp $

#TODO Get version name from keyboard. 

SET( BASEPACKAGES basedatadefs dgbbasedatadefs)
SET( PACKAGELIST basedefs dgbbasedefs dgbccbdefs dgbdsdefs dgbhcdefs
		 dgbnndefs dgbssisdefs dgbstratdefs dgbvmbdefs dgbwcpdefs
		 odgmtdefs odgprdefs odmadagascardefs develdefs ) 

INCLUDE( CMakeModules/packagescripts/ODMakePackagesUtils.cmake )

IF( APPLE )
    execute_process( COMMAND ${PSD}/data/install_files/macscripts/chfwscript ${PSD}/bin/mac ${CMAKE_INSTALL_PREFIX}/bin/mac ${CMAKE_INSTALL_PREFIX}/bin/mac
		     RESULT_VARIABLE STATUS )
    IF( NOT ${STATUS} EQUAL "0" )
	MESSAGE( FATAL_ERROR "changing dependency Failed" )
    ENDIF()
    execute_process( COMMAND ${PSD}/data/install_files/macscripts/chfwscript ${CMAKE_INSTALL_PREFIX}/bin/mac/Release ${CMAKE_INSTALL_PREFIX}/bin/mac/Release )
		     RESULT_VARIABLE STATUS )
    IF( NOT ${STATUS} EQUAL "0" )
	MESSAGE( FATAL_ERROR "changing thired party dependency Failed" )
    ENDIF()

ENDIF()

IF( APPLE OR WIN32 )
    od_sign_libs()
ENDIF()

foreach ( BASEPACKAGE ${BASEPACKAGES} )
    INCLUDE( ${PSD}/CMakeModules/packagescripts/${BASEPACKAGE}.cmake)
    init_destinationdir( ${PACK} )
    create_basepackages( ${PACK} )
endforeach()

foreach ( PACKAGE ${PACKAGELIST} )
    INCLUDE(CMakeModules/packagescripts/${PACKAGE}.cmake)
    MESSAGE( "Preparing package ${PACK}.zip ......" )
    IF( NOT DEFINED OpendTect_VERSION_MAJOR )
	MESSAGE( FATAL_ERROR "OpendTect_VERSION_MAJOR not defined" )
    ENDIF()

    IF( NOT DEFINED OD_PLFSUBDIR )
	MESSAGE( FATAL_ERROR "OD_PLFSUBDIR not defined" )
    ENDIF()

    IF( NOT DEFINED CMAKE_INSTALL_PREFIX )
	MESSAGE( FATAL_ERROR "CMAKE_INSTALL_PREFIX is not Defined. " )
    ENDIF()

    IF( ${OD_PLFSUBDIR} STREQUAL "win32" OR ${OD_PLFSUBDIR} STREQUAL "win64" )
	IF( NOT EXISTS "${PSD}/bin/win/zip.exe" )
	    MESSAGE( FATAL_ERROR "${PSD}/bin/win/zip.exe is not existed.
		     Unable to create packages.Please do an update" )
	ENDIF()
    ENDIF()

    init_destinationdir( ${PACK} )
    IF( ${PACK} STREQUAL "devel" )
        create_develpackages()
    ELSE()
	create_package( ${PACK} )
    ENDIF()
endforeach()
MESSAGE( "\n Created packages are available under ${PSD}/packages" )
