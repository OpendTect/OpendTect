#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#

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
