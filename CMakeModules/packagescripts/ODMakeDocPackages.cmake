#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to prepare documentation packages
# Author:       Nageswara
# Date:		March 2013		
#RCS:           $Id$

include( CMakeModules/packagescripts/ODMakePackagesUtils.cmake )
if( UNIX OR APPLE )
    download_packages()
else()
    message( FATAL_ERROR "Documentation packages are not prepared on Windows" )
endif()

set( DOCPACKAGES doc dgbdoc classdoc )
foreach ( PACKAGE ${DOCPACKAGES} )
    set( PACK ${PACKAGE} )
    message( "Preparing package ${PACK}.zip ......" )
    if( NOT DEFINED OpendTect_VERSION_MAJOR )
	message( FATAL_ERROR "OpendTect_VERSION_MAJOR not defined" )
    endif()

    if( NOT DEFINED CMAKE_INSTALL_PREFIX )
	message( FATAL_ERROR "CMAKE_INSTALL_PREFIX is not Defined. " )
    endif()

    if( UNIX OR APPLE )
	init_destinationdir( ${PACK} )
	create_docpackages( ${PACK} )
    endif()
endforeach()
message( "\n Created doc packages are available under ${PSD}/packages" )
