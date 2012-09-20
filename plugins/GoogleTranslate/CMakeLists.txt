#_______________________Pmake___________________________________________________
#
#	Makefile : 	GoogleTranslate plugin
# 	Feb 2012	Nageswara
#	RCS :		$Id$
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
