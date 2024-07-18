#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS General )
set( OD_FOLDER "Base" )

set( OD_MODULE_SOURCES 
	sqldatabase.cc
	sqlquery.cc
	databaseobject.cc
)

if ( OD_NO_QSQL )
    set( OD_USEQT Core )
    list( APPEND OD_MODULE_COMPILE_DEFINITIONS "OD_NO_QSQL" )
else()
    set( OD_USEQT Sql )
endif()

OD_INIT_MODULE()
