#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

ADD_DEFINITIONS( -D_CONSOLE )
set(OD_MODULE_DEPS Network )
set(OD_USEBATCH 1)
set(OD_MODULE_SOURCES
	batchprog.cc
	batchserviceservermgr.cc
)
set(OD_MODULE_LINK_OPTIONS ${OD_SUPPRESS_UNDEF_FLAGS} )
OD_INIT_MODULE()
