#_______________________Pmake___________________________________________________
#
#	Makefile : 	Batch
# 	Sep 1998	A.H. Bril
#	RCS :		$Id: CMakeLists.txt,v 1.5 2012-02-22 12:34:45 cvskris Exp $
#_______________________________________________________________________________

ADD_DEFINITIONS( -D_CONSOLE )
SET(OD_MODULE_DEPS Network )
SET(OD_USEBATCH 1)
#SET(OD_USEQT Core)
SET(OD_MODULE_SOURCES
	batchprog.cc  )
SET(OD_MODULE_LINK_OPTIONS ${OD_SUPPRESS_UNDEF_FLAGS} )
OD_INIT_MODULE()
