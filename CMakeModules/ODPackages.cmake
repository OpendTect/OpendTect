#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

macro( OD_CREATE_PACKAGE_DEFINITION )
    GET_OD_BASE_EXECUTABLES()
    configure_file( "${CMAKE_SOURCE_DIR}/CMakeModules/packagescripts/develdefs.cmake.in"
		    "${CMAKE_BINARY_DIR}/CMakeModules/packagescripts/develdefs.cmake"
		    @ONLY )

    configure_file( "${CMAKE_SOURCE_DIR}/CMakeModules/packagescripts/basedefs.cmake.in"
		    "${CMAKE_BINARY_DIR}/CMakeModules/packagescripts/basedefs.cmake"
		    @ONLY )
endmacro()

macro( OD_ADD_PACKAGES_TARGET )
    if ( NOT DEFINED PACKAGE_DIR )
	set( PACKAGE_DIR "${CMAKE_SOURCE_DIR}/../packages" )
	get_filename_component( PACKAGE_DIR "${PACKAGE_DIR}" ABSOLUTE )
    endif()

    if ( "${MAIN_GIT_BRANCH}" STREQUAL "main" )
	set( OpendTect_INST_DIR "0.0.0" )
    else()
	set( OpendTect_INST_DIR ${OpendTect_VERSION_MAJOR}.${OpendTect_VERSION_MINOR}.${OpendTect_VERSION_PATCH} )
    endif()

    get_thirdparty_targets( OD_THIRDPARTY_TARGETS ${OD_MODULES} ${OD_PLUGINS} )
    get_thirdparty_libs( "${OD_THIRDPARTY_TARGETS}" OD_THIRDPARTY_LIBS )
    set( CMAKE_FOLDER "Releases" )
    add_custom_target( packages ${CMAKE_COMMAND} 
	    -DOpendTect_INST_DIR=${OpendTect_INST_DIR}
	    -DOpendTect_FULL_VERSION=${OpendTect_FULL_VERSION}
	    -DSOURCE_DIR=${CMAKE_SOURCE_DIR}
	    -DBINARY_DIR=${CMAKE_BINARY_DIR}
	    -DOD_PLFSUBDIR=${OD_PLFSUBDIR}
	    -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
	    -DOD_DATA_INSTALL_RELPATH=${OD_DATA_INSTALL_RELPATH}
	    -DOD_LIBRARY_DIRECTORY=${OD_LIBRARY_DIRECTORY}
	    -DOD_LOCATION_DIRECTORY=${OD_LOCATION_DIRECTORY}
	    -DOD_RUNTIME_DIRECTORY=${OD_RUNTIME_DIRECTORY}
	    -DPACKAGE_DIR=${PACKAGE_DIR}
	    -DPACKAGE_TYPE=$<IF:$<CONFIG:Debug>,Devel,Production>
	    -DBUILD_USERDOC=${BUILD_USERDOC}
	    -DUSERDOC_SCRIPT_LOCATION=${USERDOC_SCRIPT_LOCATION}
	    -DUSERDOC_PROJECT=${USERDOC_PROJECT}
	    -DBUILD_DOCUMENTATION=${BUILD_DOCUMENTATION}
	    -DCLASSDOC_SCRIPT_LOCATION=${CLASSDOC_SCRIPT_LOCATION}
	    "-DOD_THIRDPARTY_LIBS=\"${OD_THIRDPARTY_LIBS}\""
	    "-DOD_THIRDPARTY_TARGETS=\"${OD_THIRDPARTY_TARGETS}\""
	    "-DOD_QTPLUGINS=\"${OD_QTPLUGINS}\""
	    -P "${CMAKE_SOURCE_DIR}/CMakeModules/packagescripts/ODMakePackages.cmake" 
	    COMMENT "Creating packages" ) 
    unset( CMAKE_FOLDER )
endmacro()


macro( OD_ADD_SIGNLIBRARIES_TARGET )
    set( CMAKE_FOLDER "Releases" )
    if ( WIN32 )
	add_custom_target( signlibraries "${SIGN_SCRIPT_LOCATION}"
			   "${CMAKE_INSTALL_PREFIX}/${OD_RUNTIME_DIRECTORY}"
			   COMMENT "Signing DLLs and EXEs" )
    elseif ( APPLE )
	add_custom_target( signlibraries "${SIGN_SCRIPT_LOCATION}"
			   "${CMAKE_INSTALL_PREFIX}"
			   COMMENT "Signing Libraries and EXEs" )
    endif()
    unset( CMAKE_FOLDER )
endmacro()
