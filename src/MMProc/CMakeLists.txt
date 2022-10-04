#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set(OD_MODULE_DEPS Network)

set(OD_MODULE_SOURCES
	batchprogtracker.cc
	jobdescprov.cc
	jobiomgr.cc
	jobrunner.cc
	initmmproc.cc
	clusterjobdispatch.cc
	mmbatchjobdispatch.cc
	mmexecproxy.cc
	mmpserver.cc
	mmpserverclient.cc
	remcommhandler.cc
	remjobexec.cc
	singlebatchjobdispatch.cc
)

set(OD_MODULE_PROGS
	od_remoteservice.cc
	od_remexec.cc
)

set( OD_ELEVATED_PERMISSIONS_PROGS od_Setup_Firewall.cc )

OD_INIT_MODULE()
