#_______________________Pmake___________________________________________________
#
#	Makefile : 	OpendTect usage management
# 	Mar 2009	Bert
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-01-23 20:55:23 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS Network )
SET(OD_MODULE_SOURCES
    odusgadm.cc odusgclient.cc odusginfo.cc odusgserver.cc )
SET(OD_MODULE_BATCHPROGS od_usage_monitor )
OD_INIT_MODULE()

