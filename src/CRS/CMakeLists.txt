#_______________________Pmake___________________________________________________
#
#	Makefile :	CRS
#	May 2017	Raman
#_______________________________________________________________________________

if ( NOT OD_NO_PROJ )
    set( OD_USEPROJ Yes )
    set( OD_USESQLITE Yes )
endif()

set( OD_MODULE_DEPS Basic )
set( OD_MODULE_SOURCES 
	    crsproj.cc
	    crssystem.cc
	    initcrs.cc )

if ( NOT OD_NO_PROJ )
    set( OD_TEST_PROGS crs.cc )
endif()

OD_INIT_MODULE()
