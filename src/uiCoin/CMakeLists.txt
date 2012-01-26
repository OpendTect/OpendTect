#_______________________Pmake___________________________________________________
#
#	Makefile : 	Coin/Ui
# 	Feb 2002	Arend Lammertink
#	RCS :		$Id: CMakeLists.txt,v 1.2 2012-01-26 20:10:40 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS visSurvey uiTools )

ADD_DEFINITIONS("-Wno-shadow -Wno-overloaded-virtual")

SET(OD_USEQT OpenGL)
SET(OD_USEOSG 1)
SET(OD_USECOIN 1)

SET(QT_MOC_HEADERS i_qthumbwhl.h)
SET(OD_MODULE_SOURCES
		keybindings.cc
		uicmain.cc
		uiprintscenedlg.cc 
		ui3dviewer.cc 
		ui3dindirectviewer.cc 
		uiosgviewer.cc 
		uistereodlg.cc 
		uithumbwheel.cc
)

SET(OD_MODULE_EXECS od_ivfileviewer od_ivfileviewer_clean )
OD_INIT_MODULE()
