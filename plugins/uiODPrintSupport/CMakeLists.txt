#________________________________________________________________________
#
# Copyright:	(C) 1995-2024 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS uiBase )
set( OD_FOLDER "OD Free Plugins" )
set( OD_IS_PLUGIN Yes )
set( OD_USEQT PrintSupport )

set( OD_MODULE_SOURCES
	uiodprinterutils.cc
	uiodprintsupportpi.cc
)

OD_INIT_MODULE()
