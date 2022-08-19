#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

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

SET(OD_MODULE_BATCHPROGS od_process_velocityconv.cc )

OD_INIT_MODULE()
