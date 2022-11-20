#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS Network )
set( OD_FOLDER "Base" )
set( OD_USEBATCH TRUE )

set( OD_MODULE_SOURCES
	batchprog.cc
	batchserviceservermgr.cc
)

set( OD_MODULE_LINK_OPTIONS ${OD_SUPPRESS_UNDEF_FLAGS} )
ADD_DEFINITIONS( -D_CONSOLE )

OD_INIT_MODULE()
