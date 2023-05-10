#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS visBase EarthModel MPEEngine Well PreStackProcessing )
set( OD_FOLDER "Base" )

set( OD_MODULE_SOURCES
	initvissurvey.cc
	visarrowdisplay.cc
	visaxisdisplay.cc
	visemobjdisplay.cc
	visemsticksetdisplay.cc
	visfaultdisplay.cc
	visfaultsetdisplay.cc
	visfaultsticksetdisplay.cc
	vishorizon2ddisplay.cc
	vishorizondisplay.cc
	visimagedisplay.cc
	vislocationdisplay.cc
	vismarchingcubessurfacedisplay.cc
	vismpeeditor.cc
	vismpeseedcatcher.cc
	vismultiattribsurvobj.cc
	vispicksetdisplay.cc
	visplanedatadisplay.cc
	vispointsetdisplay.cc
	vispolygonbodydisplay.cc
	vispolylinedisplay.cc
	visprestackdisplay.cc
	vispseventdisplay.cc
	visrandomposbodydisplay.cc
	visrandomtrackdisplay.cc
	visscalebardisplay.cc
	visseedpainter.cc
	visseis2ddisplay.cc
	vissower.cc
	vissurvobj.cc
	vissurvscene.cc
	vistransmgr.cc
	visvolumedisplay.cc
	viswelldisplay.cc
)

OD_INIT_MODULE()
