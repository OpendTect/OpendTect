#_______________________Pmake___________________________________________________
#
#	Makefile :	Distributed processing
# 	July 2008	Bert
#_______________________________________________________________________________

set(OD_MODULE_DEPS Network)

set(OD_MODULE_SOURCES
	jobdescprov.cc
	jobiomgr.cc
	jobrunner.cc
	initmmproc.cc
	clusterjobdispatch.cc
	mmbatchjobdispatch.cc
	mmproc.cc
	mmexecproxy.cc
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
