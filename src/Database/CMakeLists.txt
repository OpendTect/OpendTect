#_______________________Pmake___________________________________________________
#
#	Makefile : 	Basic utilities
# 	July 1995	A.H. Bril
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_MODULE_DEPS General )
SET(OD_MODULE_SOURCES 
	sqldatabase.cc sqlquery.cc
	mantisdatabase.cc mantistables.cc
	databaseobject.cc )

# SET(USESQL 1)

IF(USESQL)
    SET(OD_USEQT Sql)
    ADD_DEFINITIONS("-D__have_qsql__")
ELSE()
    SET(OD_USEQT Core)
ENDIF()

OD_INIT_MODULE()
