#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS uiODMain GMT )
set( OD_FOLDER "OD Free Plugins" )
set( OD_IS_PLUGIN yes )
set( OD_MODULE_SOURCES
	uigmtadv.cc
	uigmtarray2dinterpol.cc
	uigmtbasemap.cc
	uigmtclip.cc
	uigmtcoastline.cc
	uigmtcontour.cc
	uigmtfaults.cc
	uigmtlocations.cc
	uigmtmainwin.cc
	uigmtoverlay.cc
	uigmtpolyline.cc
	uigmtwells.cc
	uigmt2dlines.cc
	uigmtrandlines.cc
	uigmtsymbolpars.cc
	uigmtpi.cc
	uigmtinfodlg.cc
)

set( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
SET( OD_NO_ALO_ENTRY TRUE )

OD_INIT_MODULE()

#REFERS_QTLIBS := yes
