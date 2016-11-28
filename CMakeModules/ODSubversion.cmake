#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#_______________________________________________________________________________

if ( EXISTS ${CMAKE_SOURCE_DIR}/.svn )
    set ( OD_FROM_SVN 1 )
endif()

# the FindSubversion.cmake module is part of the standard distribution
if ( WIN32 )
    set ( CMAKE_SYSTEM_PROGRAM_PATH ${CMAKE_SYSTEM_PROGRAM_PATH}
	    "C:/Program Files/SlikSvn/bin"
	    "C:/Program Files (x86)/SlikSvn/bin" )
    #Add more likely paths if need be
endif()

include(FindSubversion)

# extract working copy information for SOURCE_DIR into MY_XXX variables
if ( Subversion_FOUND AND OD_FROM_SVN )
    Subversion_WC_INFO( ${CMAKE_SOURCE_DIR} MY )
    set ( UPDATE_CMD ${Subversion_SVN_EXECUTABLE} update )
    set ( VCS_VERSION ${MY_WC_REVISION} )
else()
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

	set ( UPDATE_CMD ${GIT_EXECUTABLE} pull --rebase )
    endif()
endif()

if ( EXISTS ${PLUGIN_DIR} )
    set ( EXTERNALPLUGINSUPDATE
	COMMAND ${UPDATE_CMD} ${PLUGIN_DIR}/../ )
endif()

if ( EXISTS ${CMAKE_SOURCE_DIR}/external/Externals.cmake )
    execute_process(
	COMMAND ${CMAKE_COMMAND}
	    -DOpendTect_DIR=${OpendTect_DIR}
	    -DUPDATE=No
	    -P external/Externals.cmake
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} )
endif()

if ( EXISTS ${CMAKE_SOURCE_DIR}/external/Externals.cmake )
    set ( EXTERNALCMD COMMAND ${CMAKE_COMMAND}
		-DOpendTect_DIR=${OpendTect_DIR}
		-DUPDATE=Yes
		-P external/Externals.cmake )
endif()

if ( EXISTS ${PLUGIN_DIR}/../external/Externals.cmake )
    set ( EXTERNALPLUGINSCMD COMMAND ${CMAKE_COMMAND}
		-DOpendTect_DIR=${OpendTect_DIR}
		-DUPDATE=Yes
		-P ${PLUGIN_DIR}/../external/Externals.cmake )
endif()

add_custom_target( update ${UPDATE_CMD}
		  ${EXTERNALPLUGINSUPDATE}
		  ${EXTERNALCMD}
		  ${EXTERNALPLUGINSCMD}
		  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}	
		  COMMENT "Updating from repositories" )
