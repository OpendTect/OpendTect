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

if( USESQL )
    set( OD_USEQT Sql )
    set( OD_MODULE_COMPILE_DEFINITIONS "__have_qsql__" )
else()
    set( OD_USEQT Core )
endif()

OD_INIT_MODULE()
