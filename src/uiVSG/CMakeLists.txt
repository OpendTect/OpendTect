#________________________________________________________________________
#
# Copyright:	(C) 1995-2026 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS uiOSG vsgSurvey )
set( OD_FOLDER "Visualization (3D)" )
set( OD_USEQT OpenGL;GuiPrivate )
set( OD_USEVSG TRUE )

if ( NOT TARGET vsgSurvey )
    get_filename_component( OD_MODULE_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME )
    list( REMOVE_ITEM OD_MODULES ${OD_MODULE_NAME} )
    set( OD_MODULES ${OD_MODULES} PARENT_SCOPE )
    return()
endif()

set( OD_MODULE_SOURCES
	uivsgcoltabdlg.cc
	uivsgviewer.cc
	uivsgcontext.cc
	uivsgwindowhandler.cc
	uivsgscenesettingsdlg.cc
	uivsgscreenshotdlg.cc
	inituivsg.cc
	uivsgwindow.cc
	vsgviewer.cc
)
if ( WIN32 OR TARGET vsgXchange::vsgXchange )
    set( OD_MODULE_PROGS od_vsgtestapp.cc )
endif()

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
