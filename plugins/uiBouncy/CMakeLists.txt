#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiBouncy plugin
# 	March 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-03-01 06:05:56 cvsnageswara Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS Bouncy uiODMain )
SET(OD_IS_PLUGIN yes)
SET(OD_USECOIN 1)
SET(OD_MODULE_SOURCES
	uibouncymain.cc
	uibouncysettingsdlg.cc
	visbouncydisplay.cc
	uibouncymgr.cc
	uibouncypi.cc)
OD_INIT_MODULE()
