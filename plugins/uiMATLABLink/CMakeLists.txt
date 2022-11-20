#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

if ( IS_DIRECTORY "${MATLAB_DIR}" )

    set( OD_MODULE_DEPS uiODMain MATLABLink )
    set( OD_FOLDER "OD Free Plugins" )
    set( OD_IS_PLUGIN yes )
    set( OD_MODULE_SOURCES
	uimatlablinkpi.cc
	uimatlabstep.cc
    )
    set( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )

    OD_INIT_MODULE()

else()

    get_filename_component( OD_MODULE_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME )
    list( REMOVE_ITEM OD_PLUGINS ${OD_MODULE_NAME} )
    set( OD_PLUGINS ${OD_PLUGINS} PARENT_SCOPE )

endif()
