#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODInternal.cmake,v 1.1 2012-03-26 15:03:57 cvskris Exp $
#_______________________________________________________________________________

SET ( OD_INSTALL_CMAKE_FILES ${OD_CMAKE_FILES} FindOpendTect OD_SetupOD )
FOREACH ( FILE ${OD_INSTALL_CMAKE_FILES} )
    INSTALL ( FILES CMakeModules/${FILE}.cmake DESTINATION CMakeModules )
ENDFOREACH()
