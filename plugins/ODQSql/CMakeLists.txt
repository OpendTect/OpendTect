#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS General )
set( OD_FOLDER "OD Free Plugins" )
set( OD_IS_PLUGIN yes )
set( OD_USEQT Sql )

set( OD_MODULE_SOURCES 
	odqsqlpi.cc
	sqldatabase.cc
	sqlquery.cc
)

OD_INIT_MODULE()
