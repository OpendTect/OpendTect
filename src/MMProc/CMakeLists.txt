#_______________________Pmake___________________________________________________
#
#	Makefile : 	Multi-Machine processing
# 	July 2008	Bert
#	RCS :		$Id: CMakeLists.txt,v 1.2 2012/03/29 11:33:26 cvskris Exp $
#_______________________________________________________________________________



SET(OD_MODULE_DEPS Network )
SET(OD_USEQT Core)
SET(OD_MODULE_SOURCES
	jobdescprov.cc jobiomgr.cc jobrunner.cc mmproc.cc mmexecproxy.cc
	remcommhandler.cc remjobexec.cc )
SET(OD_MODULE_PROGS od_remoteservice od_remexec )
OD_INIT_MODULE()
