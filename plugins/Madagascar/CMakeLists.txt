#_______________________Pmake___________________________________________________
#
#	Makefile : 	Madagascar batch-level
# 	March 2012	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-03-01 06:27:49 cvsnageswara Exp $
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
SET( OD_PLUGIN_EXECS od_madexec)
OD_INIT_MODULE()
