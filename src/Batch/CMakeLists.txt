#_______________________Pmake___________________________________________________
#
#	Makefile : 	Batch
# 	Sep 1998	A.H. Bril
#	RCS :		$Id$
#_______________________________________________________________________________


add_definitions( -DmNoAutoUiString )

ADD_DEFINITIONS( -D_CONSOLE )
set(OD_MODULE_DEPS Network )
set(OD_USEBATCH 1)
#set(OD_USEQT Core)
set(OD_MODULE_SOURCES
	batchprog.cc  )
set(OD_MODULE_LINK_OPTIONS ${OD_SUPPRESS_UNDEF_FLAGS} )
OD_INIT_MODULE()
