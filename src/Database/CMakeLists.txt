#_______________________Pmake___________________________________________________
#
#	Makefile : 	Basic utilities
# 	July 1995	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-01-11 11:43:19 cvskris Exp $
#_______________________________________________________________________________

SET(LIB_NAME Database)
SET(DEPS Basic)
SET(LIB_SOURCES 
	sqldatabase.cc sqlquery.cc
	mantisdatabase.cc mantistables.cc
	databaseobject.cc )

#SET(OD_USEQT Sql)
#ADD_DEFINITIONS("-D__have_qsql__")
OD_INIT_MODULE()
