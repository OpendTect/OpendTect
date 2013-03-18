#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Dec 2012	Nageswara
#	RCS :		$Id$
#_______________________________________________________________________________

#TODO Add date
set ( YEAR 2012 )
configure_file( ${CMAKE_SOURCE_DIR}/data/install_files/unixscripts/license_devel.txt ${CMAKE_SOURCE_DIR}/CMakeModules/templates/license.txt.in )

macro( add_licensetext DIRNAME DIRPATH )
    MESSAGE( "Installing ${DIRPATH}/${DIRNAME} " )

    execute_process( COMMAND svn checkout https://svn.opendtect.org/od/tags/od4.4.0h/${DIRPATH}/${DIRNAME} ${CMAKE_INSTALL_PREFIX}/${DIRPATH}/${DIRNAME} )
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
    FILE( GLOB CLIST ${CMAKE_SOURCE_DIR}/${DIRPATH}/${DIRNAME}/CMakeLists.txt )
    foreach( CFILE ${CLIST} )
        execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${CLIST}
			 ${CMAKE_INSTALL_PREFIX}/${DIRPATH}/${DIRNAME} )
    endforeach()
    if( ${DIRNAME} STREQUAL "visBase" )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			${CMAKE_SOURCE_DIR}/${DIRPATH}/${DIRNAME}/make.glxinfo
			${CMAKE_INSTALL_PREFIX}/${DIRPATH}/${DIRNAME} )
    endif()

    if( ${DIRNAME} STREQUAL "ODGeneral" )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			${CMAKE_SOURCE_DIR}/${DIRPATH}/${DIRNAME}/mk_specprog
			${CMAKE_INSTALL_PREFIX}/${DIRPATH}/${DIRNAME} )
    endif()
endmacro( add_licensetext )

FILE( READ ${CMAKE_SOURCE_DIR}/CMakeModules/templates/license.txt.in
	   lic_temp )
INCLUDE( ${CMAKE_SOURCE_DIR}/CMakeModules/packagescripts/develdefs.cmake )

FILE( REMOVE_RECURSE  ${CMAKE_INSTALL_PREFIX}/include
		      ${CMAKE_INSTALL_PREFIX}/src
		      ${CMAKE_INSTALL_PREFIX}/plugins
		      ${CMAKE_INSTALL_PREFIX}/soec
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
