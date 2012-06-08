#_______________________Pmake___________________________________________________
#
#	Makefile : 	HorizonAttrib plugin
# 	March 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.6 2012-06-08 11:38:46 cvsraman Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS  Basic Algo AttributeEngine PreStackProcessing
		    Attributes EarthModel MPEEngine)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	horizonattrib.cc
	horattribpi.cc
	stratamp.cc
	isopachmaker.cc)
SET( OD_MODULE_BATCHPROGS od_isopach od_stratamp)
SET( OD_PLUGIN_ALO_EXEC ${OD_ATTRIB_EXECS} )
OD_INIT_MODULE()
