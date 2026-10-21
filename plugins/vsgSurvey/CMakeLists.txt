#________________________________________________________________________
#
# Copyright:	(C) 1995-2026 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS General visBase visSurvey vsgBase )
set( OD_FOLDER "Visualization (3D)" )
set( OD_IS_PLUGIN yes )
set( OD_NO_ALO_ENTRY TRUE )
set( OD_MODULE_SOURCES
	visvsgsurvscene.cc
	visvsgsurvtransmgr.cc
)
set( OD_TEST_PROGS
	visvsgsurvey.cc
)

OD_USE_VSG()

OD_INIT_MODULE()
