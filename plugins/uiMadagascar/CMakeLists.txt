#_______________________Pmake___________________________________________________
#
#	Makefile : 	Madagascar (UI)
# 	March 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-03-01 12:28:16 cvsnageswara Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiODMain Madagascar)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	uimadagascarmain.cc
	uimadbldcmd.cc
	uimadiosel.cc
	uimadpi.cc)
OD_INIT_MODULE()

#ifdef MAD_UIMSG_IF_FAIL
#	OWNC++FLAGS := -DMAD_UIMSG_IF_FAIL
#endif
