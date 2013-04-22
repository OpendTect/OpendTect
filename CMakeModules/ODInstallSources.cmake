#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Dec 2012	Nageswara
#	RCS :		$Id$
#_______________________________________________________________________________

#TODO Add date
file( READ ${CMAKE_SOURCE_DIR}/CMakeModules/templates/license.txt.in lic_temp )
message( "CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}" )
message( "inst dir: ${CMAKE_INSTALL_PREFIX}" )
file( STRINGS ${CMAKE_SOURCE_DIR}/CMakeModules/sourcefiles.txt DEVELSTUFF )
foreach( FIL ${DEVELSTUFF} )
    get_filename_component( FPATH ${FIL} PATH )
    file( INSTALL DESTINATION ${CMAKE_INSTALL_PREFIX}/${FPATH}
	          TYPE FILE FILES ${CMAKE_SOURCE_DIR}/${FIL} )
    get_filename_component( FEXT ${FIL} EXT )
    if( ("${FEXT}" STREQUAL ".h") OR ("${FEXT}" STREQUAL ".cc") OR ("${FEXT}" STREQUAL ".c"))
	get_filename_component( FNAME ${FIL} NAME )
	file( READ ${CMAKE_INSTALL_PREFIX}/${FPATH}/${FNAME} temp )
	file( WRITE ${CMAKE_INSTALL_PREFIX}/${FPATH}/tempfile
	      "${lic_temp}\n" )
	file( APPEND ${CMAKE_INSTALL_PREFIX}/${FPATH}/tempfile
	      "${temp}\n" )
	file( RENAME
		${CMAKE_INSTALL_PREFIX}/${FPATH}/tempfile 
		${CMAKE_INSTALL_PREFIX}/${FPATH}/${FNAME} )
    endif()
endforeach()
