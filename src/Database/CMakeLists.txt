#_______________________Pmake___________________________________________________
#
#	Makefile : 	Basic utilities
# 	July 1995	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.4 2012/01/23 20:55:23 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS Basic)
SET(OD_MODULE_SOURCES 
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
