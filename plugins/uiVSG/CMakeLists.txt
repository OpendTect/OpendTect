#________________________________________________________________________
#
# Copyright:	(C) 1995-2026 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS uiODMain vsgSurvey vsgBase )
set( OD_FOLDER "Visualization (3D)" )
set( OD_IS_PLUGIN yes )
set( OD_USEQT Widgets;OpenGL;GuiPrivate )
set( OD_MODULE_SOURCES
	uivsgviewer.cc
	uivsgwindowhandler.cc
	uivsgscenesettingsdlg.cc
	uivsgscreenshotdlg.cc
	uivsgpi.cc
	uivsgwindow.cc
	vsgviewer.cc
)
if ( WIN32 OR TARGET vsgXchange::vsgXchange )
    set( OD_MODULE_PROGS od_vsgtestapp.cc )
endif()
set( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )

OD_USE_VSG()

if ( UNIX )
    if ( NOT TARGET Fontconfig::Fontconfig OR NOT TARGET Freetype::Freetype )
	message( SEND_ERROR
	    "Fontconfig and Freetype are required to build text on Linux" )
    else()
	list( APPEND OD_MODULE_EXTERNAL_LIBS
	      Fontconfig::Fontconfig Freetype::Freetype )
    endif()
endif()

if ( OD_MODULE_PROGS )
    get_target_property( Qt6Gui_INCLUDE_DIRS Qt6::Gui INTERFACE_INCLUDE_DIRECTORIES )
    foreach( dir ${Qt6Gui_INCLUDE_DIRS} )
	if ( NOT "${dir}" MATCHES "Gui$" )
	    continue()
	endif()
	if ( EXISTS "${dir}/${QT_VERSION}/QtGui/rhi" )
	    list( APPEND OD_MODULE_INCLUDESYSPATH "${dir}/${QT_VERSION}/QtGui" )
	    list( APPEND OD_MODULE_INCLUDESYSPATH "${dir}/${QT_VERSION}/QtGui/rhi" )
	endif()
    endforeach()
endif()

OD_INIT_MODULE()

if ( TARGET od_vsgtestapp )
    if ( WIN32 )
	target_link_libraries( od_vsgtestapp dwrite )
    endif()
    if ( UNIX AND TARGET vsgXchange::vsgXchange )
	target_link_libraries( od_vsgtestapp vsgXchange::vsgXchange )
    endif()
    if ( UNIX AND NOT APPLE AND TARGET Fontconfig::Fontconfig )
	target_link_libraries( od_vsgtestapp Fontconfig::Fontconfig )
    endif()
endif()
