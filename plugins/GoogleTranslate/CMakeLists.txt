#_______________________Pmake___________________________________________________
#
#	Makefile : 	GoogleTranslate plugin
# 	Feb 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.4 2012/03/29 11:33:24 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiODMain)
SET(OD_USEQT Core)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	googletranslatepi.cc
	googletranslator.cc
	uitranslatedlg.cc)
SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
OD_INIT_MODULE()
