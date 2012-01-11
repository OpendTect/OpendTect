#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODQtUtils.cmake,v 1.1 2012-01-11 11:43:19 cvskris Exp $
#_______________________________________________________________________________

IF(ENV{OD_QTDIR})
    SET(QTDIR ENV{OD_QTDIR})
    SET(ENV{QTDIR} ${QTDIR} )
ELSE()
    IF(ENV{QTDIR})
        SET(QTDIR ENV{OD_QTDIR})
        SET(ENV{OD_QTDIR} ${QTDIR})
    ELSE()
        SET(QTDIR "" CACHE STRING "QT4 location")
        SET(ENV{QTDIR} ${QTDIR} )
        SET(ENV{OD_QTDIR} ${QTDIR})
    ENDIF()
ENDIF()

FIND_PACKAGE(Qt4 REQUIRED QtGui QtCore QtSql )
