#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to build a release
# Author:       K. Tingdahl
# Date:		August 2012		
#RCS:           $Id$

#add_custom_target( do_install ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target install
#        WORKING_DIRECTORY ${CMAKE_BINARY_DIR} )

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
#	DEPENDS do_install
        COMMENT "Creating packages" ) 

