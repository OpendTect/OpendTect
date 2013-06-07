#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Dec 2012	Nageswara
#	RCS :		$Id$
#_______________________________________________________________________________

#TODO Add date
set ( YEAR 2012 )
configure_file( ${CMAKE_SOURCE_DIR}/data/install_files/unixscripts/license_devel.txt ${CMAKE_SOURCE_DIR}/CMakeModules/templates/license.txt.in )

set( ROOT "https://svn.opendtect.org/od" )
IF( NOT DEFINED ISTAG )
    MESSAGE( FATAL_ERROR "ISTAG is not defined" )
ENDIF()

IF( "${ISTAG}" STREQUAL "" )
    MESSAGE( FATAL_ERROR "ISTAG is not defined" )
ENDIF()

IF( NOT DEFINED OD_BRANCH )
    MESSAGE( FATAL_ERROR "OD_BRANCH is not defined" )
ENDIF()

IF( "${OD_BRANCH}" STREQUAL "" )
    MESSAGE( FATAL_ERROR "OD_BRANCH is not defined" )
ENDIF()

IF( "${ISTAG}" STREQUAL "N" OR "${ISTAG}" STREQUAL "n" )
    set( ROOT "${ROOT}/branches" )
ELSEIF( "${ISTAG}" STREQUAL "Y" OR "${ISTAG}" STREQUAL "y" )
    set( ROOT "${ROOT}/tags" )
ENDIF()

set( ROOT "${ROOT}/${OD_BRANCH}" )
macro( add_licensetext DIRNAME DIRPATH )
    MESSAGE( "Installing ${DIRPATH}/${DIRNAME} " )

message( "root:${ROOT}/${DIRPATH}/${DIRNAME}" )
message( "${CMAKE_INSTALL_PREFIX}/${DIRPATH}/${DIRNAME}" )

    execute_process( COMMAND svn checkout ${ROOT}/${DIRPATH}/${DIRNAME} ${CMAKE_INSTALL_PREFIX}/${DIRPATH}/${DIRNAME} RESULT_VARIABLE STATUS )
    IF( NOT ${STATUS} EQUAL "0" )
	MESSAGE( FATAL_ERROR "Failed to checkout ${ROOT}/${DIRPATH}/${DIRNAME}" )
    ENDIF()

    FILE( REMOVE_RECURSE ${CMAKE_INSTALL_PREFIX}/${DIRPATH}/${DIRNAME}/.svn)

    FILE( GLOB HFILES ${CMAKE_INSTALL_PREFIX}/${DIRPATH}/${DIRNAME}/*.h )
    FILE( GLOB SFILES ${CMAKE_INSTALL_PREFIX}/${DIRPATH}/${DIRNAME}/*.cc )
    FILE( GLOB CFILES ${CMAKE_INSTALL_PREFIX}/${DIRPATH}/${DIRNAME}/*.c )
    SET( FILES ${HFILES} ${SFILES} ${CFILES} )

    foreach( FIL ${FILES} )
	FILE( READ ${FIL} temp )
	FILE( WRITE ${CMAKE_INSTALL_PREFIX}/${DIRPATH}/${DIRNAME}/tempfile
	      "${lic_temp}\n" )
	FILE( APPEND ${CMAKE_INSTALL_PREFIX}/${DIRPATH}/${DIRNAME}/tempfile
	      "${temp}\n" )
	get_filename_component( FILENAME ${FIL} NAME )
	FILE( RENAME
		${CMAKE_INSTALL_PREFIX}/${DIRPATH}/${DIRNAME}/tempfile 
		${CMAKE_INSTALL_PREFIX}/${DIRPATH}/${DIRNAME}/${FILENAME} )
    endforeach()
endmacro( add_licensetext )

FILE( READ ${CMAKE_SOURCE_DIR}/CMakeModules/templates/license.txt.in
	   lic_temp )
INCLUDE( ${CMAKE_SOURCE_DIR}/CMakeModules/packagescripts/develdefs.cmake )

FILE( REMOVE_RECURSE  ${CMAKE_INSTALL_PREFIX}/include
		      ${CMAKE_INSTALL_PREFIX}/src
		      ${CMAKE_INSTALL_PREFIX}/plugins
		      ${CMAKE_INSTALL_PREFIX}/spec
		      ${CMAKE_INSTALL_PREFIX}/tests )

foreach( INCLUDEDIR ${INCLIBLIST} )
    add_licensetext( "${INCLUDEDIR}" "include" )
endforeach()

foreach( SRCDIR ${SRCLIBLIST} )
    add_licensetext( "${SRCDIR}" "src" )
endforeach()

foreach( PLUGINDIR ${PLUGINS} )
    add_licensetext( "${PLUGINDIR}" "plugins" )
endforeach()

foreach( SPECDIR ${SPECSOURCES} )
    add_licensetext( "${SPECDIR}" "spec" )
endforeach()

foreach( TESTDIR ${TESTS} )
    add_licensetext( "${TESTDIR}" "tests" )
endforeach()
