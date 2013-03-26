#_______________________Pmake___________________________________________________
#
#	Makefile : 	OpendTect usage management
# 	Mar 2009	Bert
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_MODULE_DEPS Network )
SET(OD_MODULE_SOURCES
    odusgadm.cc odusgclient.cc odusginfo.cc odusgserver.cc )
SET(OD_MODULE_BATCHPROGS od_usage_monitor.cc )
OD_INIT_MODULE()

