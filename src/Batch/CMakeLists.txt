#_______________________Pmake___________________________________________________
#
#	Makefile : 	Batch
# 	Sep 1998	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.4 2012-02-10 16:11:14 cvskris Exp $
#_______________________________________________________________________________

ADD_DEFINITIONS("-D__prog__")


SET(OD_MODULE_DEPS Network )
SET(OD_USEPROG 1)
#SET(OD_USEQT Core)
SET(OD_MODULE_SOURCES
	batchprog.cc  )
SET(OD_MODULE_LINK_OPTIONS ${OD_SUPPRESS_UNDEF_FLAGS} )
OD_INIT_MODULE()
