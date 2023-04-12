#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

if ( NOT OD_NO_PROJ )
    set( OD_LINKPROJ Yes )
    set( OD_USESQLITE Yes )
endif()

set( OD_MODULE_DEPS General )
set( OD_FOLDER "OD Free Plugins" )
set( OD_IS_PLUGIN yes )
set( OD_MODULE_SOURCES
	crspi.cc
	crsproj.cc
	crssystem.cc
)

if ( NOT OD_NO_PROJ )
    set( OD_TEST_PROGS crs.cc )
    # Auto-loading done by the 'General' module
endif()

OD_INIT_MODULE()
