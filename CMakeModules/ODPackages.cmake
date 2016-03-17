#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to build a release
# Author:       K. Tingdahl
# Date:		August 2012		


macro( OD_CREATE_DEVEL_PACKAGE_DEFINITION )
    configure_file( ${CMAKE_SOURCE_DIR}/CMakeModules/packagescripts/develdefs.cmake.in
		    ${CMAKE_BINARY_DIR}/CMakeModules/packagescripts/develdefs.cmake
		    @ONLY )
endmacro()


macro( OD_ADD_PACKAGES_TARGET )
    if ( NOT DEFINED PACKAGE_DIR )
	set( PACKAGE_DIR ${CMAKE_SOURCE_DIR}/packages )
    endif()

    add_custom_target( packages  ${CMAKE_COMMAND} 
	    -DOpendTect_VERSION_MAJOR=${OpendTect_VERSION_MAJOR} 
	    -DOpendTect_VERSION_MINOR=${OpendTect_VERSION_MINOR} 
	    -DOpendTect_VERSION_PATCH=${OpendTect_VERSION_PATCH} 
	    -DOpendTect_FULL_VERSION=${OpendTect_FULL_VERSION}
	    "-DOD_THIRD_PARTY_LIBS=\"${OD_THIRD_PARTY_LIBS}\""
	    "-DOD_THIRD_PARTY_LIBS_DEBUG=\"${OD_THIRD_PARTY_LIBS_DEBUG}\""
	    -DOD_PLFSUBDIR=${OD_PLFSUBDIR} 
	    -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} 
	    -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
	    -DPACKAGE_DIR=${PACKAGE_DIR}
	    -DBINARY_DIR=${CMAKE_BINARY_DIR}
	    -DBREAKPAD_DIR=${BREAKPAD_DIR}
	    -DOD_ENABLE_BREAKPAD=${OD_ENABLE_BREAKPAD}
	    -DBUILD_DOCUMENTATION=${BUILD_DOCUMENTATION}
	    -DBUILD_USERDOC=${BUILD_USERDOC}
	    -DUSERDOC_PROJECT=${USERDOC_PROJECT}
	    -DMATLAB_DIR=${MATLAB_DIR}
	    -P ${CMAKE_SOURCE_DIR}/CMakeModules/packagescripts/ODMakePackages.cmake 
	    COMMENT "Creating packages" ) 
endmacro()
