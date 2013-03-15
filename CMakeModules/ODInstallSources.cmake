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
    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
		   ${CMAKE_SOURCE_DIR}/${DIRPATH}/${DIRNAME}
		   ${CMAKE_INSTALL_PREFIX}/${DIRPATH}/${DIRNAME} )
    file( REMOVE_RECURSE ${CMAKE_INSTALL_PREFIX}/${DIRPATH}/${DIRNAME}/.svn )
    file( REMOVE_RECURSE ${CMAKE_INSTALL_PREFIX}/${DIRPATH}/${DIRNAME}/CMakeFiles )
    FILE( GLOB HFILES ${CMAKE_INSTALL_PREFIX}/${DIRPATH}/${DIRNAME}/*.h )
    FILE( GLOB SFILES ${CMAKE_INSTALL_PREFIX}/${DIRPATH}/${DIRNAME}/*.cc )
    SET( FILES ${HFILES} ${SFILES} )

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
