#_______________________Pmake___________________________________________________
#
#	Makefile : 	Texture plugin (UI)
# 	Sep 2012	Paul
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_DEPS uiAttributes TextureAttrib)
SET(OD_MODULE_SOURCES 
	uitextureattrib.cc
	uitextureattribpi.cc)
SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
OD_INIT_MODULE()
