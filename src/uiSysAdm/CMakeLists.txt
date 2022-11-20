#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS uiIo )
set( OD_FOLDER "Base (GUI)" )

set( OD_MODULE_SOURCES
	uiodsysadm.cc
	uiodsysadmcoltabs.cc
)

set( OD_MODULE_PROGS od_sysadmmain.cc  )

OD_INIT_MODULE()
