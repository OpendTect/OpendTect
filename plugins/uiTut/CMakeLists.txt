#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS uiODMain Tut )
set( OD_FOLDER "OD Free Plugins" )
set( OD_IS_PLUGIN yes )
if ( NOT "${OD_SUBSYSTEM}" STREQUAL "od" )
    set( OD_USEQT Widgets )
endif()
set( OD_MODULE_SOURCES
	uitutseistools.cc
	uituthortools.cc
	uitutorialattrib.cc
	uitutvolproc.cc
	uitutwelltools.cc
	uitutpi.cc
)
set( OD_MODULE_GUI_PROGS tut_app.cc )
set( OD_PLUGIN_ALO_EXEC
	${OD_MAIN_EXEC}
	tut_app )

if ( "${OD_SUBSYSTEM}" STREQUAL "od" )
    set( OD_NO_ALO_ENTRY TRUE )  # OpendTect SDK internal build
endif()

OD_INIT_MODULE()

if ( NOT "${OD_SUBSYSTEM}" STREQUAL "od" )
    set( PLUGIN_VERSION ${${OD_MODULE_NAME}_VERSION_MAJOR}.${${OD_MODULE_NAME}_VERSION_MINOR}.${${OD_MODULE_NAME}_VERSION_PATCH} )
    string( TOLOWER ${OD_MODULE_NAME} MOD_PREFIX )
    configure_file( "${CMAKE_SOURCE_DIR}/CMakeModules/version.h.in"
                    "${CMAKE_CURRENT_BINARY_DIR}/${MOD_PREFIX}version.h" )
    unset( MOD_PREFIX )
    string( REPLACE ":\\" ":/" CMAKE_INSTALL_PREFIX_ed "${CMAKE_INSTALL_PREFIX}" )
    install( CODE "
              file( WRITE \"${CMAKE_INSTALL_PREFIX_ed}/relinfo/ver.${${OD_MODULE_NAME}_PACKAGE}_${OD_PLFSUBDIR}.txt\" \"${PLUGIN_VERSION}\n\" ) " )
    unset( CMAKE_INSTALL_PREFIX_ed )
    unset( PLUGIN_VERSION )
endif()
