#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

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
