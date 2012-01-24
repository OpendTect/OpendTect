#_______________________Pmake___________________________________________________
#
#	Makefile : 	Batch
# 	Sep 1998	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.2 2012-01-24 13:51:14 cvskris Exp $
#_______________________________________________________________________________

ADD_DEFINITIONS("-D__prog__")


SET(OD_MODULE_DEPS Network )
SET(OD_USE_PROG 1)
#SET(OD_USEQT Core)
SET(OD_MODULE_SOURCES
	batchprog.cc  )
SET(OD_MODULE_LINK_OPTIONS "-flat_namespace -undefined suppress" )
OD_INIT_MODULE()
