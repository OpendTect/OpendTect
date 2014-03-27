#_______________________Pmake___________________________________________________
#
#	Makefile : 	Coin/Ui
# 	Feb 2002	Arend Lammertink
#	RCS :		$Id$
#_______________________________________________________________________________

SET(OD_MODULE_DEPS visSurvey uiTools )
SET(OD_USEQT OpenGL)
SET(OD_USEOSG 1)

SET(OD_MODULE_SOURCES
		inituiosg.cc
		uiosgfont.cc
		uiprintscenedlg.cc
		ui3dviewer.cc 
		ui3dindirectviewer.cc 
		uistereodlg.cc 
)

SET(OD_MODULE_PROGS od_osgfileviewer.cc )
OD_INIT_MODULE()
