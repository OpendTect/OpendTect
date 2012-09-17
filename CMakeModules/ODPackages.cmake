#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to build a release
# Author:       K. Tingdahl
# Date:		August 2012		
#RCS:           $Id: ODPackages.cmake,v 1.6 2012/09/11 12:26:50 cvsnageswara Exp $

add_custom_target( packages  ${CMAKE_COMMAND} 
        -DOpendTect_VERSION_MAJOR=${OpendTect_VERSION_MAJOR} 
        -DOpendTect_VERSION_MINOR=${OpendTect_VERSION_MINOR} 
        -DOpendTect_VERSION_PATCH=${OpendTect_VERSION_PATCH} 
        -DOD_PLFSUBDIR=${OD_PLFSUBDIR} 
#        -DPACKAGE_DEFINITIONS=${PACKAGE_DEFINITIONS} 
        -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} 
	-DPSD=${PROJECT_SOURCE_DIR}
	-DQTDIR=${QTDIR}
	-DCOINDIR=${COINDIR}
        -P ${PROJECT_SOURCE_DIR}/CMakeModules/packagescripts/ODMakePackages.cmake 
        COMMENT "Creating packages" ) 

