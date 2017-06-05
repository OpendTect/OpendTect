#_______________________Pmake___________________________________________________
#
#	Makefile :	CRS
#	May 2017	Raman
#	RCS :		$Id$
#_______________________________________________________________________________

set( OD_USEPROJ4 1 )
set( OD_MODULE_DEPS Basic)
set( OD_IS_PLUGIN yes)
set( OD_MODULE_SOURCES 
	    crsproj.cc
	    crssystem.cc
	    crspi.cc )

set( OD_TEST_PROGS
	    crs.cc )

set ( OD_PLUGIN_ALO_EXEC ${OD_ATTRIB_EXECS} ${OD_VOLUME_EXECS} ${OD_SEIS_EXECS}
    			 ${OD_PRESTACK_EXECS} ${OD_ZAXISTRANSFORM_EXECS} )

OD_INIT_MODULE()
