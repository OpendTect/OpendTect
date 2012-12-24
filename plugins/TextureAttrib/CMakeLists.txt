#_______________________Pmake___________________________________________________
#
#	Makefile : 	Texture plugin (base)
# 	Sep 2012	Paul
#	RCS :		$Id$
#_______________________________________________________________________________

#Setup the module
SET(OD_MODULE_DEPS AttributeEngine EarthModel Well)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	textureattrib.cc
	textureattribpi.cc)
SET( OD_PLUGIN_ALO_EXEC ${OD_ATTRIB_EXECS} )
OD_INIT_MODULE()
