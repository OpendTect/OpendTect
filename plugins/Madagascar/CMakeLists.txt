#_______________________Pmake___________________________________________________
#
#	Makefile : 	Madagascar batch-level
# 	March 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.8 2012/06/21 05:21:08 cvsraman Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS AttributeEngine)
SET(OD_IS_PLUGIN yes)
SET(OD_MODULE_SOURCES
	madio.cc
	maddefs.cc
	madproc.cc
	madprocflow.cc
	madprocexec.cc
	madpi.cc madstream.cc)
SET ( OD_MODULE_BATCHPROGS od_madexec )
OD_INIT_MODULE()
