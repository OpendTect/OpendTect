#_______________________Pmake___________________________________________________
#
#	Makefile : 	Coin/Ui
# 	Feb 2002	Arend Lammertink
#	RCS :		$Id$
#_______________________________________________________________________________

add_definitions( -DmNoAutoUiString )
set(OD_MODULE_DEPS visSurvey uiTools )
set(OD_USEQT OpenGL)
set(OD_USEOSG 1)

set(OD_MODULE_SOURCES
		inituiosg.cc
		keybindings.cc
		uiosgfont.cc
		uiprintscenedlg.cc
		ui3dviewer.cc 
		ui3dindirectviewer.cc 
		uistereodlg.cc 
)

set(OD_MODULE_PROGS od_osgfileviewer.cc )
OD_INIT_MODULE()
