#_______________________Pmake___________________________________________________
#
#	Makefile : 	SEG-Y UI
# 	June 2015	Bert
#_______________________________________________________________________________

FIND_OD_PLUGIN( "uiSEGYTools" )

if ( uiSEGYTools_FOUND )

    set(OD_MODULE_DEPS uiSEGYTools uiODMain)
    set(OD_IS_PLUGIN yes)
    set(OD_MODULE_SOURCES
	uisegypi.cc
    )

    set( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
    OD_INIT_MODULE()
endif()
