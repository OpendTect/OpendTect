#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

SET(OD_MODULE_DEPS uiTools )
SET(OD_MODULE_SOURCES
	canvascommands.cc
	cmdcomposer.cc
	cmddriverbasics.cc
	cmddriver.cc
	cmdfunction.cc
	cmdrecorder.cc
	command.cc
	drivercommands.cc
	identifierman.cc
	inputcommands.cc
	interpretexpr.cc
	listcommands.cc
	menubutcommands.cc
	objectfinder.cc
	qtcommands.cc
	searchkey.cc
	tablecommands.cc
	treecommands.cc
	uicmddriverdlg.cc
	uicmddrivermgr.cc
)
OD_INIT_MODULE()
