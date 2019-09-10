#_______________________Pmake___________________________________________________
#
#	Makefile :	COLOP (UI)
#	Sep 2019	Khushnood
#	RCS :		$Id$
#_______________________________________________________________________________
IF(WIN32)
    SET(OD_MODULE_DEPS uiODMain)
    SET(OD_IS_PLUGIN yes)
    SET(OD_MODULE_SOURCES
	    uicoloppi.cc)

    SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
    OD_INIT_MODULE()
ENDIF()
