#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Dec 2012	Nageswara
#	RCS :		$Id$
#_______________________________________________________________________________

#TODO Add date
configure_file( ${CMAKE_SOURCE_DIR}/data/install_files/unixscripts/license_devel.txt ${CMAKE_SOURCE_DIR}/CMakeModules/templates/license.txt.in )

macro( add_licensetext DIRNAME DIRPATH )
    message( "Installing ${DIRPATH}/${DIRNAME} " )
    file( GLOB HFILES ${CMAKE_SOURCE_DIR}/${DIRPATH}/${DIRNAME}/*.h )
    file( GLOB SFILES ${CMAKE_SOURCE_DIR}/${DIRPATH}/${DIRNAME}/*.cc )
    set( FILES ${HFILES} ${SFILES} )

    foreach( FIL ${FILES} )
	file( READ ${FIL} temp )
	file( WRITE ${CMAKE_INSTALL_PREFIX}/${DIRPATH}/${DIRNAME}/tempfile
	      "${lic_temp}\n" )
	file( APPEND ${CMAKE_INSTALL_PREFIX}/${DIRPATH}/${DIRNAME}/tempfile
	      "${temp}\n" )
	get_filename_component( FILENAME ${FIL} NAME )
	file( RENAME
		${CMAKE_INSTALL_PREFIX}/${DIRPATH}/${DIRNAME}/tempfile 
		${CMAKE_INSTALL_PREFIX}/${DIRPATH}/${DIRNAME}/${FILENAME} )
    endforeach()
endmacro( add_licensetext )

file( READ ${CMAKE_SOURCE_DIR}/CMakeModules/templates/license.txt.in
	   lic_temp )
include( ${CMAKE_SOURCE_DIR}/CMakeModules/packagescripts/develdefs.cmake )
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
