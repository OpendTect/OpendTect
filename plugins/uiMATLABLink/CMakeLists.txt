#_______________________Pmake___________________________________________________
#
#	Makefile : 	uiMATLAB plugin
# 	March 2012	Nanne Hemstra
#	RCS :		$Id$
#_______________________________________________________________________________

if ( NOT MATLAB_DIR STREQUAL "" )

    SET(OD_MODULE_DEPS uiODMain MATLABLink )
    SET(OD_IS_PLUGIN yes)
    SET(OD_MODULE_SOURCES
	uimatlablinkpi.cc
	uimatlabstep.cc
    )
    SET( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
    OD_INIT_MODULE()
else()
    get_filename_component( OD_MODULE_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME )
    list( REMOVE_ITEM OD_PLUGINS ${OD_MODULE_NAME} )
    set( OD_PLUGINS ${OD_PLUGINS} PARENT_SCOPE )
endif()
