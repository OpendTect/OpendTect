#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS Seis uiFlatView uiODMain Madagascar )
set( OD_FOLDER "OD Free Plugins" )
set( OD_IS_PLUGIN yes )
set( OD_MODULE_SOURCES
	uitutodmad.cc
	uitutodmadpi.cc
)
set( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
set( OD_NO_ALO_ENTRY TRUE )

OD_INIT_MODULE()
