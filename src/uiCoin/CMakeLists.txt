#_______________________Pmake___________________________________________________
#
#	Makefile : 	Coin/Ui
# 	Feb 2002	Arend Lammertink
#	RCS :		$Id: CMakeLists.txt,v 1.7 2012/04/12 04:36:48 cvsranojay Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS visSurvey uiTools )
SET(OD_USEQT OpenGL)
SET(OD_USECOIN 1)

SET(QT_MOC_HEADERS i_qthumbwhl.h)
SET(OD_MODULE_SOURCES
		keybindings.cc
		uicmain.cc
		uiprintscenedlg.cc 
		ui3dviewer.cc 
		uistereodlg.cc 
		uithumbwheel.cc
)

SET( OD_MODULE_PROGS od_ivfileviewer_clean )
SET( OD_MODULE_GUI_PROGS od_ivfileviewer )
OD_INIT_MODULE()
