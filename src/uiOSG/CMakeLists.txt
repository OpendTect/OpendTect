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
	od3dviewer.cc
	odfontimplementation.cc
	odgraphicswindow.cc
	ui3dviewer.cc
	ui3dviewerbody.cc
	uiglinfo.cc
	uiosgfont.cc
	uiprintscenedlg.cc
	uistereodlg.cc
)

if ( OD_USE_QOPENGL )
    list( APPEND OD_MODULE_SOURCES
	odopenglwidget.cc
	odosgviewer.cc
    )
endif()

set( OD_MODULE_PROGS od_osgfileviewer.cc )

OD_INIT_MODULE()
