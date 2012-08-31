#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODZlibUtils.cmake,v 1.2 2012-08-31 11:33:02 cvssalil Exp $
#_______________________________________________________________________________

IF(UNIX OR WIN32)
    FIND_PACKAGE( Zlib REQUIRED )
ENDIF()
