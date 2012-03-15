#_______________________Pmake___________________________________________________
#
#	Makefile : 	HorizonAttrib plugin
# 	March 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.3 2012-03-15 10:49:15 cvsbert Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS  Batch Basic Algo AttributeEngine PreStackProcessing
		    Attributes EarthModel MPEEngine)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	horizonattrib.cc
	horattribpi.cc
	stratamp.cc
	isopachmaker.cc)
SET( OD_MODULE_BATCHPROGS od_isopach od_stratamp)
SET( OD_PLUGIN_EXECS ${OD_MAIN_EXEC} ${OD_ATTRIB_EXECS} )
OD_INIT_MODULE()
