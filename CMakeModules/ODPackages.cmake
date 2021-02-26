#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to build a release
# Author:       K. Tingdahl
# Date:		August 2012		


macro( OD_CREATE_PACKAGE_DEFINITION )
    configure_file( ${CMAKE_SOURCE_DIR}/CMakeModules/packagescripts/develdefs.cmake.in
		    ${CMAKE_BINARY_DIR}/CMakeModules/packagescripts/develdefs.cmake
		    @ONLY )
endmacro()


macro( OD_ADD_PACKAGES_TARGET )
    if ( NOT DEFINED PACKAGE_DIR )
	set( PACKAGE_DIR ${CMAKE_SOURCE_DIR}/packages )
    endif()
    FIND_OD_PLUGIN( "ODHDF5" )
    if ( ODHDF5_FOUND )
	set( INCLUDE_ODHDF5 "YES" )
    else()
	set( INCLUDE_ODHDF5 "NO" )
    endif()

    if ( EXISTS "${OD_BINARY_DEBUG_BASEDIR}" )
	 set( ONLY_SET_VAR ON )
	include ( "${OD_BINARY_DEBUG_BASEDIR}/CMakeModules/FindOpendTect.cmake" )
	set( OD_THIRD_PARTY_FILES_DEBUG ${OD_THIRD_PARTY_FILES} )
	include ( "${OD_BINARY_BASEDIR}/CMakeModules/FindOpendTect.cmake" )
	unset( ONLY_SET_VAR )
    endif()
    add_custom_target( packages  ${CMAKE_COMMAND} 
	    -DOpendTect_VERSION_MAJOR=${OpendTect_VERSION_MAJOR} 
	    -DOpendTect_VERSION_MINOR=${OpendTect_VERSION_MINOR} 
	    -DOpendTect_VERSION_PATCH=${OpendTect_VERSION_PATCH} 
	    -DOpendTect_FULL_VERSION=${OpendTect_FULL_VERSION}
	    "-DOD_THIRD_PARTY_FILES=\"${OD_THIRD_PARTY_FILES}\""
	    "-DOD_THIRD_PARTY_FILES_DEBUG=\"${OD_THIRD_PARTY_FILES_DEBUG}\""
	    "-DOD_QTPLUGINS=\"${OD_QTPLUGINS}\""
	    "-DOD_QT_TRANSLATION_FILES=\"${OD_QT_TRANSLATION_FILES}\""
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
	    -DQT_VERSION_MAJOR=${QT_VERSION_MAJOR}
	    -DINCLUDE_ODHDF5=${INCLUDE_ODHDF5}
	    -P ${CMAKE_SOURCE_DIR}/CMakeModules/packagescripts/ODMakePackages.cmake 
	    COMMENT "Creating packages" ) 
endmacro()
