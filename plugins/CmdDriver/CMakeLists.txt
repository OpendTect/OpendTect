#_______________________Pmake___________________________________________________
#
#	Makefile : 	CmdDriver plugin
# 	Jan 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.3 2012/03/29 11:33:24 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiODMain)
SET(OD_IS_PLUGIN yes)
SET( OD_USECOIN 1 )
SET(OD_USEQT 1)
SET(OD_MODULE_SOURCES
	canvascommands.cc
	cmdcomposer.cc
	cmddriver.cc
	cmddriverbasics.cc
	cmddriverpi.cc
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
	cmddriverdlg.cc
	cmddrivermgr.cc)

SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )

OD_INIT_MODULE()
