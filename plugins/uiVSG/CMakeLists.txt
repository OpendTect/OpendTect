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
	    "Fontconfig and Freetype are required to build text on Unix" )
    else()
	list( APPEND OD_MODULE_EXTERNAL_LIBS
	      Fontconfig::Fontconfig Freetype::Freetype )
    endif()
endif()

if ( UNIX AND TARGET vsgXchange::vsgXchange )
    list( APPEND OD_MODULE_EXTERNAL_LIBS vsgXchange::vsgXchange )
endif()

OD_INIT_MODULE()
