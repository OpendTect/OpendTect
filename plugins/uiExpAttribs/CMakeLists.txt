#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS ExpAttribs uiODMain )
set( OD_FOLDER "OD Free Plugins" )
set( OD_IS_PLUGIN yes )
set( OD_MODULE_SOURCES
	uieventfreqattrib.cc
	uiexpattribspi.cc
	uigrubbsfilterattrib.cc
	uicorrmultiattrib.cc
	uisimilaritybyaw.cc
	uiintegratedtrace.cc
)
set( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )

OD_INIT_MODULE()
