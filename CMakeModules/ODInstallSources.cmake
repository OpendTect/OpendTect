#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Dec 2012	Nageswara
#	RCS :		$Id$
#_______________________________________________________________________________

file( STRINGS ${CMAKE_SOURCE_DIR}/CMakeModules/sourcefiles_od.txt DEVELSTUFF )
foreach( FIL ${DEVELSTUFF} )
    get_filename_component( FPATH ${FIL} PATH )
    file( INSTALL DESTINATION ${CMAKE_INSTALL_PREFIX}/${FPATH}
	          TYPE FILE FILES ${CMAKE_SOURCE_DIR}/${FIL} )
endforeach()
