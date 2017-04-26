#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiCRS
# 	May 2017	Raman
#	RCS :		$Id$
#_______________________________________________________________________________

option( BUILD_CRS "Build the CRS/uiCRS plugins" OFF )
if ( BUILD_CRS )
    set( OD_MODULE_DEPS uiODMain CRS )
    set( OD_IS_PLUGIN yes)
    set( OD_MODULE_SOURCES 
		uicrspi.cc )

    set ( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
    OD_INIT_MODULE()
endif()
