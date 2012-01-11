#_______________________Pmake___________________________________________________
#
#	Makefile : 	Basic utilities
# 	July 1995	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.2 2012-01-11 12:00:29 cvskris Exp $
#_______________________________________________________________________________

SET(LIB_NAME Database)
SET(DEPS Basic)
SET(LIB_SOURCES 
	sqldatabase.cc sqlquery.cc
	mantisdatabase.cc mantistables.cc
	databaseobject.cc )

#SET(USESQL 1)

IF(USESQL)
    SET(OD_USEQT Sql)
    ADD_DEFINITIONS("-D__have_qsql__")
ELSE()
    SET(OD_USEQT Core)
ENDIF()

OD_INIT_MODULE()
