#_______________________Pmake___________________________________________________
#
#	Makefile : 	GoogleTranslate plugin
# 	Feb 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-02-29 13:09:22 cvsnageswara Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiODMain)
SET(OD_USEQT Core)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	googletranslatepi.cc
	googletranslator.cc
	uitranslatedlg.cc)
OD_INIT_MODULE()

#USEQT := yes
