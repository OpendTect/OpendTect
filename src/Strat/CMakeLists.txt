#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS General )
set( OD_FOLDER "Base" )

set( OD_MODULE_SOURCES
	initstrat.cc
	stratlith.cc
	stratlayer.cc
	stratlayermodel.cc
	stratlayersequence.cc
	stratreftree.cc
	stratseqattrib.cc
	stratseqgen.cc
	stratunit.cc
	stratunititer.cc
	stratunitrepos.cc
)

set( OD_TEST_PROGS
	layergen.cc
        layermodel.cc
)

OD_INIT_MODULE()
