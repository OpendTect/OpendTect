#_______________________Pmake___________________________________________________
#
#	Makefile : 	Basic UI-classes
# 	July 1995	A.H. Bril
#	RCS :		$Id$
#_______________________________________________________________________________

set(OD_MODULE_DEPS General)
set(OD_USEQT Widgets)
set(QT_MOC_HEADERS i_qaction.h
		i_qbutton.h
		i_qcombobox.h
		i_qdial.h
		i_qlineedit.h
		i_qlistbox.h
		i_qmdiarea.h
		i_qmenu.h
		i_qslider.h
		i_qspinbox.h
		i_qsystemtrayicon.h
		i_qtabbar.h
		i_qtable.h
		i_qtextedit.h
		i_qtoolbar.h
		i_qtoolbut.h
		i_qtreeview.h
		i_qtxtbrowser.h)
SET(OD_MODULE_SOURCES     i_layout.cc
		i_layoutitem.cc
		inituibase.cc
		pixmap.cc
		uiaction.cc
		uibutton.cc
		uibuttongroup.cc
		uicolor.cc
		uicombobox.cc
		uicursor.cc
		uidesktopservices.cc
		uidate.cc
		uidial.cc
		uidockwin.cc
		uieventfilter.cc
		uifiledlg.cc
		uifont.cc
		uigroup.cc
		uigraphicscoltab.cc
		uigraphicsitem.cc
		uigraphicsitemimpl.cc
		uigraphicsscene.cc
		uigraphicsviewbase.cc
		uilabel.cc
		uilayout.cc
		uilineedit.cc
		uilistbox.cc
		uimain.cc
		uimainwin.cc
		uimdiarea.cc
		uimenu.cc
		uimsg.cc
		uiobj.cc
		uiobjectstack.cc
		uiobjbody.cc
		odgraphicsitem.cc
		uiprogressbar.cc
		uirgbarray.cc
		uirubberband.cc
		uiseparator.cc
		uishortcutsmgr.cc
		uislider.cc
		uispinbox.cc
		uisplashscreen.cc
		uisplitter.cc
		uistatusbar.cc
		uisystemtrayicon.cc
		uitabbar.cc
		uitable.cc
		uitabstack.cc
		uitextedit.cc
		uitoolbar.cc
		uitreeview.cc
		uivirtualkeyboard.cc)
SET(OD_MODULE_GUI_PROGS od_DispMsg.cc  )
OD_ADD_SOURCE_FILES( uimainicon.xpm )
OD_INIT_MODULE()
