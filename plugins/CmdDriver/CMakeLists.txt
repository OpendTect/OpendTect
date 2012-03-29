#_______________________Pmake___________________________________________________
#
#	Makefile : 	GapDecon plugin
# 	Feb 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.2 2012-03-29 11:26:24 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiODMain)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	canvascommands.cc
	cmdcomposer.cc
	cmddriverbasics.cc
	cmddriver.cc
	cmddriverdlg.cc
	cmddrivermgr.cc
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
	cmddriverpi.cc)
SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
OD_INIT_MODULE()
