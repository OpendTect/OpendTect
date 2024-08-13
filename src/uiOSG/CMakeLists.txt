#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS visSurvey uiTools )
set( OD_FOLDER "Base (GUI)" )
set( OD_USEQT OpenGL )
set( OD_USEOSG TRUE )

set( OD_MODULE_SOURCES
	inituiosg.cc
	keybindings.cc
	odfontimplementation.cc
	odopenglwidget.cc
	ui3dviewer.cc
	uiglinfo.cc
	uiosgfont.cc
	uiprintscenedlg.cc
	uistereodlg.cc
)

if ( OD_USE_QOPENGL )
    list( APPEND OD_MODULE_SOURCES
	odosgviewer.cc
	od3dviewer.cc
    )
    list( APPEND OD_MODULE_COMPILE_DEFINITIONS "OD_USE_QOPENGL" )
else()
    list( APPEND OD_MODULE_SOURCES
	odgraphicswindow.cc
	ui3dviewerbody.cc
    )

    set( OD_MODULE_PROGS od_osgfileviewer.cc )
endif()

OD_INIT_MODULE()
