#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to build a release
# Author:       K. Tingdahl
# Date:		August 2012		
#RCS:           $Id: ODMakePackages.cmake,v 1.9 2012/09/11 12:25:44 cvsnageswara Exp $

#TODO Get version name from keyboard. 

SET( BASEPACKAGES basedatadefs )
SET( PACKAGELIST basedefs dgbccbdefs dgbdsdefs dgbhcdefs dgbnndefs dgbssisdefs dgbstratdefs dgbvmbdefs dgbwcpdefs odgmtdefs odgprdefs odmadagascardefs ) 

INCLUDE( CMakeModules/packagescripts/ODMakePackagesUtils.cmake )

foreach ( BASEPACKAGE ${BASEPACKAGES} )
    INCLUDE(CMakeModules/packagescripts/${BASEPACKAGE}.cmake)
    init_destinationdir( ${PACK} )
    create_basepackages( ${PACK} )
endforeach()

foreach ( PACKAGE ${PACKAGELIST} )
    INCLUDE(CMakeModules/packagescripts/${PACKAGE}.cmake)
#    INCLUDE( CMakeModules/packagescripts/ODMakePackagesUtils.cmake )
    IF( NOT DEFINED OpendTect_VERSION_MAJOR )
	MESSAGE( FATAL_ERROR "OpendTect_VERSION_MAJOR not defined" )
    ENDIF()

    IF( NOT DEFINED OD_PLFSUBDIR )
	MESSAGE( FATAL_ERROR "OD_PLFSUBDIR not defined" )
    ENDIF()

    IF( NOT EXISTS ${PSD}/inst )
	MESSAGE( FATAL_ERROR "${PSD}/inst is not existed. Do make install. " )
    ENDIF()

#    SET( REL_DIR "${OpendTect_VERSION_MAJOR}.${OpendTect_VERSION_MINOR}" )
    IF( APPLE )
	SET( REL_DIR "OpendTect${OpendTect_VERSION_MAJOR.${OpendTect_VERSION_MINOR}}.app" )
    ENDIF( APPLE )
    
    init_destinationdir( ${PACK} )
    create_package( ${PACK} )
endforeach()
