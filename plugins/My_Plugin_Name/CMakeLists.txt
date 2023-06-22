#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

FIND_OD_PLUGIN( "uiODMain" )

if ( uiODMain_FOUND )

    set( OD_MODULE_DEPS uiODMain )
    set( OD_FOLDER "OD Free Plugins" )
    set( OD_IS_PLUGIN yes )
    set( OD_MODULE_SOURCES
	my_plugin_name_pi.cc
	my_first_separate_source.cc
    )

    set( OD_NO_ALO_ENTRY TRUE )
    set( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )

    OD_INIT_MODULE()

else()

    get_filename_component( OD_MODULE_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME )
    list( REMOVE_ITEM PLUGINS ${OD_MODULE_NAME} )
    set( PLUGINS ${PLUGINS} PARENT_SCOPE )

endif()
