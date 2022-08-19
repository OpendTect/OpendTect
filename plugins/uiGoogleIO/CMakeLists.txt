#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

SET(OD_MODULE_DEPS uiODMain)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	uigoogleexpsurv.cc
	uigoogleexpwells.cc
	uigoogleexp2dlines.cc
	uigoogleexppolygon.cc
	uigoogleexprandline.cc
	uigoogleiopi.cc
	uigisexpdlg.cc
)
SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )

OD_INIT_MODULE()
