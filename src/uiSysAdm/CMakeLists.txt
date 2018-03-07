#_______________________Pmake___________________________________________________
#
#	Makefile : 	ui - System Administrator tools
# 	Jul 2000	Bert
#_______________________________________________________________________________

set(OD_MODULE_DEPS uiIo )
set(OD_MODULE_SOURCES uiodsysadm.cc uiodsysadmcoltabs.cc )

set( OD_MODULE_PROGS od_sysadmmain.cc  )

OD_INIT_MODULE()
