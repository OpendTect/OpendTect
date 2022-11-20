#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS EarthModel AttributeEngine )
set( OD_FOLDER "Base" )
set( OD_MODULE_SOURCES
	emhor2dseisiter.cc
	gridcreator.cc
	hor2dfrom3dcreator.cc
	horizonattrib.cc
	seis2deventsnapper.cc
	stratamp.cc
	initemattrib.cc
)

set( OD_MODULE_BATCHPROGS
	od_process_2dgrid.cc
	od_process_attrib_em.cc
	od_stratamp.cc
)

OD_INIT_MODULE()
