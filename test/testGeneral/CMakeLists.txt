#_______________________Pmake___________________________________________________
#
#	Makefile : 	General utilities
# 	July 1995	A.H. Bril
#	RCS :		$Id$
#_______________________________________________________________________________

SET ( OD_MODULE_DEPS General )
SET ( OD_MODULE_TEST_LABEL General )

if ( NOT DEFINED WIN32 )
    SET ( OD_TEST_PROGS testibmformat )
endif()
OD_INIT_MODULE()


