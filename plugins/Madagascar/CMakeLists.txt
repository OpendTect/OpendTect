#_______________________Pmake___________________________________________________
#
#	Makefile : 	Madagascar batch-level
# 	March 2012	Nageswara
#_______________________________________________________________________________

set(OD_MODULE_DEPS AttributeEngine)
set(OD_IS_PLUGIN yes)
set(OD_MODULE_SOURCES
	madio.cc
	maddefs.cc
	madproc.cc
	madprocflow.cc
	madprocexec.cc
	madpi.cc madstream.cc)
set( OD_MODULE_BATCHPROGS od_madexec.cc )
OD_INIT_MODULE()
