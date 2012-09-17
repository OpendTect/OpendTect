#_______________________Pmake___________________________________________________
#
#	Makefile : 	ui - Seis
# 	Jul 2000	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.3 2012/03/29 11:33:27 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiIo )
SET(OD_MODULE_SOURCES uiodsysadm.cc uiodsysadmcoltabs.cc )

SET( OD_MODULE_PROGS od_sysadmmain )

OD_INIT_MODULE()
