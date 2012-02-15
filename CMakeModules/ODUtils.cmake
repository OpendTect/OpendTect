#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODUtils.cmake,v 1.4 2012-02-15 10:01:05 cvskris Exp $
#_______________________________________________________________________________

#Empty ModDeps-file
SET( OD_MODDEPS_FILE ${CMAKE_BINARY_DIR}/Pmake/ModDeps.od )
FILE(WRITE ${OD_MODDEPS_FILE} "")

#Discover 64 or 32 bits
IF(CMAKE_SIZEOF_VOID_P MATCHES "8")
    SET( OD_64BIT 1 )
ENDIF()

#Discover 64 or 32 bits
IF( ${CMAKE_BUILD_TYPE} MATCHES Debug)
    SET( OD_DEBUG 1 )
    SET( OD_OUTPUTDIR "G" )
ELSE()
    SET( OD_DEBUG )
    SET( OD_OUTPUTDIR "O" )
ENDIF()


