#_______________________Pmake___________________________________________________
#
#	Makefile : 	ui - Seis
# 	Jul 2000	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-01-23 20:55:23 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiIo )
SET(OD_MODULE_SOURCES uiodsysadm.cc uiodsysadmcoltabs.cc )

SET( OD_MODULE_BATCHPROGS od_sysadmmain )

OD_INIT_MODULE()
