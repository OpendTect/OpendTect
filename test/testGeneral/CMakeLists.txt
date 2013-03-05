#_______________________Pmake___________________________________________________
#
#	Makefile : 	General utilities
# 	July 1995	A.H. Bril
#	RCS :		$Id$
#_______________________________________________________________________________

set ( OD_MODULE_DEPS General )

if ( NOT DEFINED WIN32 )
    SET ( OD_TEST_PROGS ibmformat )
endif()

OD_INIT_MODULE()


