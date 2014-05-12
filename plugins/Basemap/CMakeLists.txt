#_______________________Pmake___________________________________________________
#
#	Makefile : 	GMT plugin
# 	Feb 2012	Nageswara
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_MODULE_DEPS EarthModel Seis Well)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	basemappi.cc
	basemapgeom2d.cc
	basemaprandomline.cc
	basemapwell.cc
)
OD_INIT_MODULE()
