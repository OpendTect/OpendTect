#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id$
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
else()
    set ( MY_WC_REVISION 0 )
    set ( MY_WC_URL "" )
endif()

if ( OD_FROM_SVN )
    set ( UPDATE_CMD ${Subversion_SVN_EXECUTABLE} update )
    if ( EXISTS ${CMAKE_SOURCE_DIR}/external/Externals.cmake )
	execute_process(
	    COMMAND ${CMAKE_COMMAND}
		-DOpendTect_DIR=${OpendTect_DIR}
		-DNO_UPDATE=Yes
		-P external/Externals.cmake
	    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} )
    endif()
endif()

if ( EXISTS ${CMAKE_SOURCE_DIR}/external/Externals.cmake )
    set ( EXTERNALCMD COMMAND ${CMAKE_COMMAND}
		-DOpendTect_DIR=${OpendTect_DIR}
		-P external/Externals.cmake )
endif()

add_custom_target( update ${UPDATECMD}
		  ${EXTERNALCMD}
		  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}	
		  COMMENT "Updating from repositories" )
