#_______________________Pmake___________________________________________________
#
#	Makefile : 	VMB::VelocityFunction
# 	2008		Kris
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012/01/23 20:55:23 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS AttributeEngine EarthModel )
SET(OD_MODULE_SOURCES
		initvelocity.cc
		velocityfunction.cc
		velocityfunctioninterval.cc
		velocityfunctiongrid.cc
		velocityfunctionvolume.cc
		velocityfunctionascio.cc
		velocityfunctionstored.cc
		velocitypicks.cc
		velocitypicksundo.cc
		velocityvolumeconversion.cc
     )

SET(OD_MODULE_BATCHPROGS od_process_velocityconv )
OD_INIT_MODULE()

