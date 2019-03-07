#_______________________Pmake___________________________________________________
#
#	Makefile :	uiCRS
#	May 2017	Raman
#	RCS :		$Id$
#_______________________________________________________________________________

set( OD_MODULE_DEPS uiODMain CRS )
set( OD_IS_PLUGIN yes)
set( OD_MODULE_SOURCES 
	    uicrssystem.cc
	    uicrspi.cc )

set ( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} ${OD_SURVEY_EXECS} )
OD_INIT_MODULE()
