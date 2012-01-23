#_______________________Pmake___________________________________________________
#
#	Makefile : 	Batch
# 	Sep 1998	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012-01-23 20:55:23 cvskris Exp $
#_______________________________________________________________________________

ADD_DEFINITIONS("-D__prog__")
SET(CMAKE_SHARED_LINKER_FLAGS "-flat_namespace -undefined suppress")


SET(OD_MODULE_DEPS Network )
SET(OD_USE_PROG 1)
#SET(OD_USEQT Core)
SET(OD_MODULE_SOURCES
	batchprog.cc  )
OD_INIT_MODULE()
