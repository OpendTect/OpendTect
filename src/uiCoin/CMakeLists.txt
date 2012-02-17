#_______________________Pmake___________________________________________________
#
#	Makefile : 	Coin/Ui
# 	Feb 2002	Arend Lammertink
#	RCS :		$Id: CMakeLists.txt,v 1.4 2012-02-17 09:21:44 cvskris Exp $
#_______________________________________________________________________________

SET(OD_MODULE_DEPS visSurvey uiTools )
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
