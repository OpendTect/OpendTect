#_______________________Pmake___________________________________________________
#
#	Makefile :	CRS
#	May 2017	Raman
#_______________________________________________________________________________

set( OD_USEPROJ4 Yes )
set( OD_MODULE_DEPS Basic )
set( OD_MODULE_SOURCES 
	    crsproj.cc
	    crssystem.cc
	    initcrs.cc )

set( OD_TEST_PROGS
	    crs.cc )

OD_INIT_MODULE()
