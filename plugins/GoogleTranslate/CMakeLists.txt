#_______________________Pmake___________________________________________________
#
#	Makefile : 	GoogleTranslate plugin
# 	Feb 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.2 2012-03-01 08:37:18 cvsnageswara Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiODMain)
SET(OD_USEQT Core)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	googletranslatepi.cc
	googletranslator.cc
	uitranslatedlg.cc)
OD_INIT_MODULE()
