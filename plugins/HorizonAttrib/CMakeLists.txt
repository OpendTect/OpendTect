#_______________________Pmake___________________________________________________
#
#	Makefile : 	HorizonAttrib plugin
# 	March 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.2 2012-03-06 13:45:20 cvskris Exp $
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
OD_INIT_MODULE()
