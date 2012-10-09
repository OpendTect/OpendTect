#_______________________Pmake___________________________________________________
#
#	Makefile : 	ui - Seis
# 	Jul 2000	A.H. Bril
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_MODULE_DEPS uiIo )
SET(OD_MODULE_SOURCES uiodsysadm.cc uiodsysadmcoltabs.cc )

SET( OD_MODULE_PROGS od_sysadmmain )

OD_INIT_MODULE()
