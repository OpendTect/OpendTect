#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to build a release
# Author:       K. Tingdahl
# Date:		August 2012		
#RCS:           $Id$


macro( OD_CREATE_DEVEL_PACKAGE_DEFINITION )
    configure_file( ${CMAKE_SOURCE_DIR}/CMakeModules/packagescripts/develdefs.cmake.in
		    ${CMAKE_BINARY_DIR}/CMakeModules/packagescripts/develdefs.cmake
		    @ONLY )
endmacro()


macro( OD_ADD_PACKAGES_TARGET )
    add_custom_target( packages  ${CMAKE_COMMAND} 
	    -DOpendTect_VERSION_MAJOR=${OpendTect_VERSION_MAJOR} 
	    -DOpendTect_VERSION_MINOR=${OpendTect_VERSION_MINOR} 
	    -DOpendTect_VERSION_DETAIL=${OpendTect_VERSION_DETAIL}
	    -DOpendTect_VERSION_PATCH=${OpendTect_VERSION_PATCH} 
	    "-DOD_THIRD_PARTY_LIBS=\"${OD_THIRD_PARTY_LIBS}\""
	    "-DOD_THIRD_PARTY_LIBS_DEBUG=\"${OD_THIRD_PARTY_LIBS_DEBUG}\""
	    -DOD_PLFSUBDIR=${OD_PLFSUBDIR} 
	    -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} 
	    -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
	    -DBINARY_DIR=${CMAKE_BINARY_DIR}
	    -DBREAKPAD_DIR=${BREAKPAD_DIR}
	    -DCODESIGN_COMMAND=${CODESIGN_COMMAND}
	    -P ${CMAKE_SOURCE_DIR}/CMakeModules/packagescripts/ODMakePackages.cmake 
	    DEPENDS sources
	    COMMENT "Creating packages" ) 
endmacro()
