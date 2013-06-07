#_______________________Pmake___________________________________________________
#
#	Makefile : 	CmdDriver plugin
# 	Jan 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.3 2012/03/29 11:33:24 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiODMain uiCmdDriver)
SET(OD_IS_PLUGIN yes)
SET( OD_USECOIN 1 )
SET(OD_USEQT 1)
SET(OD_MODULE_SOURCES
	coincommands.cc
	cmddriverpi.cc
)
SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )

OD_INIT_MODULE()
