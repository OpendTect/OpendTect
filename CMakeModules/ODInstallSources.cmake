#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Dec 2012	Nageswara
#	RCS :		$Id$
#_______________________________________________________________________________

#TODO Add date
file( READ ${BINARY_DIR}/CMakeModules/license.txt lic_temp )
message( "BINARY_DIR: ${BINARY_DIR}" )
file( STRINGS ${BINARY_DIR}/CMakeModules/sourcefiles_od.txt DEVELSTUFF )
foreach( FIL ${DEVELSTUFF} )
    get_filename_component( FPATH ${FIL} PATH )
    if ( EXISTS ${CMAKE_SOURCE_DIR}/${FIL} )
	file( INSTALL DESTINATION ${CMAKE_INSTALL_PREFIX}/${FPATH}
	      TYPE FILE FILES ${CMAKE_SOURCE_DIR}/${FIL} )
    elseif( EXISTS ${BINARY_DIR}/${FIL} )
	file( INSTALL DESTINATION ${CMAKE_INSTALL_PREFIX}/${FPATH}
	      TYPE FILE FILES ${BINARY_DIR}/${FIL} )
    endif()
    get_filename_component( FEXT ${FIL} EXT )
    if( ("${FEXT}" STREQUAL ".h") OR ("${FEXT}" STREQUAL ".cc") OR ("${FEXT}" STREQUAL ".c"))
	get_filename_component( FNAME ${FIL} NAME )
	file( READ ${CMAKE_INSTALL_PREFIX}/${FPATH}/${FNAME} temp )
	file( WRITE ${CMAKE_INSTALL_PREFIX}/${FPATH}/tempfile
	      "${lic_temp}${OD_LINESEP}" )
	file( APPEND ${CMAKE_INSTALL_PREFIX}/${FPATH}/tempfile
	      "${temp}${OD_LINESEP}" )
	file( RENAME
		${CMAKE_INSTALL_PREFIX}/${FPATH}/tempfile 
		${CMAKE_INSTALL_PREFIX}/${FPATH}/${FNAME} )
    endif()
endforeach()
