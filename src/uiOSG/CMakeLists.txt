#_______________________Pmake___________________________________________________
#
#	Makefile : 	Coin/Ui
# 	Feb 2002	Arend Lammertink
#_______________________________________________________________________________

set(OD_MODULE_DEPS visSurvey uiTools )
set(OD_USEQT OpenGL)
set(OD_USEOSG Yes)

SET(OD_MODULE_SOURCES
	inituiosg.cc
	keybindings.cc
	od3dviewer.cc
	odfontimplementation.cc
	odgraphicswindow.cc
	odopenglwidget.cc
	ui3dindirectviewer.cc
	ui3dviewer.cc
	uiglinfo.cc
	uiosgfont.cc
	uiprintscenedlg.cc
	uistereodlg.cc 
)

SET(OD_MODULE_PROGS od_osgfileviewer.cc )
OD_INIT_MODULE()
