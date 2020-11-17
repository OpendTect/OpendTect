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
	odfontimplementation.cc
	odgraphicswindow.cc
	uiglinfo.cc
	uiosgfont.cc
	uiprintscenedlg.cc
	ui3dviewer.cc 
	ui3dindirectviewer.cc 
	uistereodlg.cc 
)

SET(OD_MODULE_PROGS od_osgfileviewer.cc )
OD_INIT_MODULE()
