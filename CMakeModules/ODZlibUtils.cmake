#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id$
#_______________________________________________________________________________

IF(UNIX OR WIN32)
    FIND_PACKAGE( Zlib REQUIRED )
ENDIF()
