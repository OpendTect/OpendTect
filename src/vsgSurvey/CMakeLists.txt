#________________________________________________________________________
#
# Copyright:	(C) 1995-2026 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

if ( NOT TARGET vsgBase )
    get_filename_component( OD_MODULE_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME )
    list( REMOVE_ITEM OD_MODULES ${OD_MODULE_NAME} )
    set( OD_MODULES ${OD_MODULES} PARENT_SCOPE )
    return()
endif()

set( OD_MODULE_DEPS vsgBase visSurvey )
set( OD_FOLDER "Visualization (3D)" )
set( OD_USEVSG TRUE )
set( OD_MODULE_SOURCES
	visvsgsurvscene.cc
	visvsgsurvtransmgr.cc
	initvsgsurvey.cc
)

set( OD_TEST_PROGS visvsgsurvey.cc )

OD_INIT_MODULE()
