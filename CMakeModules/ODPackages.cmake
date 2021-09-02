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
	include ( "${CMAKE_CURRENT_BINARY_DIR}/CMakeModules/FindOpendTect.cmake" )
	set( OD_THIRD_PARTY_FILES_DEBUG ${OD_THIRD_PARTY_FILES} )
	include ( "${CMAKE_CURRENT_BINARY_DIR}/CMakeModules/FindOpendTect.cmake" )
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
	    -DQT_VERSION_MAJOR=${QT_VERSION_MAJOR}
	    -DOD_PLFSUBDIR=${OD_PLFSUBDIR} 
	    -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
	    -DBINARY_DIR=${CMAKE_BINARY_DIR}
	    -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} 
	    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
	    -DINCLUDE_ODHDF5=${INCLUDE_ODHDF5}
	    -DMATLAB_DIR=${MATLAB_DIR}
	    -DOD_ENABLE_BREAKPAD=${OD_ENABLE_BREAKPAD}
	    -DBREAKPAD_DIR=${BREAKPAD_DIR}
	    -DPACKAGE_DIR=${PACKAGE_DIR}
	    -DBUILD_DOCUMENTATION=${BUILD_DOCUMENTATION}
	    -DBUILD_USERDOC=${BUILD_USERDOC}
	    -DUSERDOC_PROJECT=${USERDOC_PROJECT}
	    -P ${CMAKE_SOURCE_DIR}/CMakeModules/packagescripts/ODMakePackages.cmake 
	    COMMENT "Creating packages" ) 
endmacro()


macro( OD_ADD_SIGNLIBRARIES_TARGET )
    if ( WIN32 )
	    set( SIGNSCRIPTPATH "${CMAKE_SOURCE_DIR}/bin/${OD_PLFSUBDIR}/sign_binaries_win64.cmd" )
	    set( EXECPLFDIR "${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/${CMAKE_BUILD_TYPE}" )
	    file( TO_NATIVE_PATH ${EXECPLFDIR}  EXECPLFDIR )
	    add_custom_target( signlibraries ${SIGNSCRIPTPATH} ${EXECPLFDIR}
			       COMMENT "Signing DLLs and EXEs" )
    endif()
endmacro()
