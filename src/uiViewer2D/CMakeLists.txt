#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS uiMPE )
set( OD_FOLDER "Base (GUI)" )

set( OD_MODULE_SOURCES
	inituiviewer2d.cc
	view2ddata.cc
	view2ddataman.cc
	view2dfault.cc
	view2dfaultss2d.cc
	view2dfaultss3d.cc
	view2dhorizon2d.cc
	view2dhorizon3d.cc
	view2dpickset.cc
	view2dseismic.cc
)

OD_INIT_MODULE()
