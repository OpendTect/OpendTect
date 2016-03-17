#_______________________Pmake___________________________________________________
#
#	Makefile : 	VMB::VelocityFunction
# 	2008		Kris
#_______________________________________________________________________________


set(OD_MODULE_DEPS AttributeEngine EarthModel )
set(OD_MODULE_SOURCES
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

set(OD_MODULE_BATCHPROGS od_process_velocityconv.cc )
OD_INIT_MODULE()
