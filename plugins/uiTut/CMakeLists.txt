#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS uiODMain Tut )
set( OD_FOLDER "OD Free Plugins" )
set( OD_IS_PLUGIN yes )
set( OD_MODULE_SOURCES
	uitutseistools.cc
	uituthortools.cc
	uitutorialattrib.cc
	uitutvolproc.cc
	uitutwelltools.cc
	uitutpi.cc
)
set( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
if ( "${OD_SUBSYSTEM}" STREQUAL "od" )
    set( OD_NO_ALO_ENTRY TRUE ) # OpendTect SDK internal build
endif()

OD_INIT_MODULE()
