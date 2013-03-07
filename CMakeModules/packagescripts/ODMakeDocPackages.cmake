#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to prepare documentation packages
# Author:       Nageswara
# Date:		January 2013		
#RCS:           $Id$

INCLUDE( CMakeModules/packagescripts/ODMakePackagesUtils.cmake )
IF( UNIX OR APPLE )
    download_packages()
ENDIF()

SET( DOCPACKAGES doc dgbdoc )
foreach ( PACKAGE ${DOCPACKAGES} )
    SET( PACK ${PACKAGE} )
    MESSAGE( "Preparing package ${PACK}.zip ......" )
    IF( NOT DEFINED OpendTect_VERSION_MAJOR )
	MESSAGE( FATAL_ERROR "OpendTect_VERSION_MAJOR not defined" )
    ENDIF()

    IF( NOT DEFINED CMAKE_INSTALL_PREFIX )
	MESSAGE( FATAL_ERROR "CMAKE_INSTALL_PREFIX is not Defined. " )
    ENDIF()

    IF( WIN32 )
	IF( NOT EXISTS "${PSD}/bin/win/unzip.exe" )
	    MESSAGE( FATAL_ERROR "${PSD}/bin/win/zip.exe is not existed.
		     Unable to create packages.Please do an update" )
	ENDIF()
    ENDIF()

    IF( UNIX OR APPLE )
	init_destinationdir( ${PACK} )
	create_docpackages( ${PACK} )
    ENDIF()
endforeach()
MESSAGE( "\n Created doc packages are available under ${PSD}/packages" )
