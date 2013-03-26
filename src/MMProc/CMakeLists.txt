#_______________________Pmake___________________________________________________
#
#	Makefile : 	Multi-Machine processing
# 	July 2008	Bert
#	RCS :		$Id$
#_______________________________________________________________________________



SET(OD_MODULE_DEPS Network )
SET(OD_USEQT Core)
SET(OD_MODULE_SOURCES
	jobdescprov.cc jobiomgr.cc jobrunner.cc mmproc.cc mmexecproxy.cc
	remcommhandler.cc remjobexec.cc )
SET(OD_MODULE_PROGS od_remoteservice.cc od_remexec.cc )
OD_INIT_MODULE()
