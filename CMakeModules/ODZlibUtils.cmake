#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODZlibUtils.cmake,v 1.1 2012-02-01 11:50:49 cvskris Exp $
#_______________________________________________________________________________

IF(UNIX)
    FIND_PACKAGE( Zlib REQUIRED )
ENDIF()
