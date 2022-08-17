#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#

set(OD_MODULE_DEPS uiODMain)
set(OD_IS_PLUGIN yes)
set(OD_USEPYTHON yes)
set(OD_MODULE_SOURCES
	presentationspec.cc
	uipresentationmaker.cc
	uipresentationmakerpi.cc
)

set( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
OD_INIT_MODULE()
