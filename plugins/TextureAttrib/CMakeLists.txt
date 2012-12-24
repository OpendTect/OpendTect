#_______________________Pmake___________________________________________________
#
#	Makefile : 	Texture plugin (base)
# 	Sep 2012	Paul
#	RCS :		$Id: CMakeLists.txt 26287 2012-09-20 14:30:37Z bert.bril@dgbes.com $
#_______________________________________________________________________________

#Setup the module
SET(OD_MODULE_DEPS AttributeEngine EarthModel Well)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	textureattrib.cc
	textureattribpi.cc)
SET( OD_PLUGIN_ALO_EXEC ${OD_ATTRIB_EXECS} )
OD_INIT_MODULE()
