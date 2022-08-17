#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#

if ( EXISTS ${CMAKE_SOURCE_DIR}/.git )
    set ( OD_FROM_GIT 1 )
endif()
if ( EXISTS ${CMAKE_SOURCE_DIR}/.svn )
    set ( OD_FROM_SVN 1 )
endif()

if ( OD_FROM_SVN )
    # the FindSubversion.cmake module is part of the standard distribution
    if ( WIN32 )
	set ( CMAKE_SYSTEM_PROGRAM_PATH ${CMAKE_SYSTEM_PROGRAM_PATH}
		"C:/Program Files/SlikSvn/bin"
		"C:/Program Files (x86)/SlikSvn/bin" )
	#Add more likely paths if need be
    endif()

    include(FindSubversion)
endif() # OD_FROM_SVN

set ( VCS_BRANCH "unknown" )
set ( VCS_BRANCH_DEF )

if ( Subversion_FOUND AND OD_FROM_SVN )
    # extract working copy information for SOURCE_DIR into MY_XXX variables
    Subversion_WC_INFO( ${CMAKE_SOURCE_DIR} MY )
    set ( UPDATE_CMD ${Subversion_SVN_EXECUTABLE} update )
    set ( VCS_VERSION ${MY_WC_REVISION} )
elseif( OD_FROM_GIT )
    set ( VCS_VERSION 0 )

    find_package(Git)

    if( GIT_FOUND )
	# Get the latest abbreviated commit hash of the working branch
	execute_process(
	  COMMAND ${GIT_EXECUTABLE} log -1 --format=%h
	  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	  OUTPUT_VARIABLE VCS_VERSION
	  OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	execute_process(
	  COMMAND git rev-parse --abbrev-ref HEAD
	  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	  OUTPUT_VARIABLE VCS_BRANCH
	  OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	if ( VCS_BRANCH STREQUAL "main" )
	    set ( VCS_BRANCH_DEF "#define mVCS_DEVEL" )
	else()
	    set ( VCS_BRANCH_DEF "#define mVCS_STABLE" )
	endif()

	set ( UPDATE_CMD ${GIT_EXECUTABLE} pull )
    endif()
endif()

macro( OD_SETUP_EXTERNALS )

    if ( EXISTS "${CMAKE_SOURCE_DIR}/external/Externals.cmake" )
	execute_process(
	    COMMAND ${CMAKE_COMMAND}
		-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
		-DOpendTect_DIR=${OpendTect_DIR}
		-DOD_NO_OSG=${OD_NO_OSG}
		-DPROJ_DIR=${PROJ_DIR}
		-DOD_NO_PROJ=${OD_NO_PROJ}
		-DBUILD_PROJ=${BUILD_PROJ}
		-DUPDATE=No
		-P "${CMAKE_SOURCE_DIR}/external/Externals.cmake"
	    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	    ERROR_VARIABLE ERROUTPUT
	    RESULT_VARIABLE STATUS )
	if ( NOT ${STATUS} EQUAL 0 )
	    message( FATAL_ERROR "${ERROUTPUT}" )
	endif()

	set ( EXTERNALCMD COMMAND ${CMAKE_COMMAND}
		    -DOpendTect_DIR=${OpendTect_DIR}
		    -DOD_NO_OSG=${OD_NO_OSG}
		    -DPROJ_DIR=${PROJ_DIR}
		    -DOD_NO_PROJ=${OD_NO_PROJ}
		    -DBUILD_PROJ=${BUILD_PROJ}
		    -DUPDATE=Yes
		    -P external/Externals.cmake )
    endif()

    if ( NOT "${UPDATE_CMD}" STREQUAL "" )

	add_custom_target( update
		      ${UPDATE_CMD} ${EXTERNALCMD}
		      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}	
		      COMMENT "Updating from repositories" )

    endif()

endmacro( OD_SETUP_EXTERNALS )
