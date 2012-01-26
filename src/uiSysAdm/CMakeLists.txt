#_______________________Pmake___________________________________________________
#
#	Makefile : 	ui - Seis
# 	Jul 2000	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.2 2012-01-26 20:13:47 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiIo )
SET(OD_MODULE_SOURCES uiodsysadm.cc uiodsysadmcoltabs.cc )

SET( OD_MODULE_EXECS od_sysadmmain )

OD_INIT_MODULE()
