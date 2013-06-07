#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODZlibUtils.cmake,v 1.2 2012/03/19 13:42:45 cvskris Exp $
#_______________________________________________________________________________

IF(UNIX)
    FIND_PACKAGE( Zlib REQUIRED )
ENDIF()
