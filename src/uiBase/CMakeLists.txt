#_______________________Pmake___________________________________________________
#
#	Makefile : 	Basic UI-classes
# 	July 1995	A.H. Bril
#_______________________________________________________________________________

set(OD_MODULE_DEPS General )
set(OD_USEQT PrintSupport)
set(QT_MOC_HEADERS
	i_qaction.h
	i_qbutton.h
	i_qcombobox.h
	i_qdial.h
	i_qlineedit.h
	i_qlistbox.h
	i_qmdiarea.h
	i_qptr.h
	i_qslider.h
	i_qspinbox.h
	i_qsystemtrayicon.h
	i_qtabbar.h
	i_qtable.h
	i_qtableview.h
	i_qtextedit.h
	i_qtoolbar.h
	i_qtreeview.h
)

set(OD_MODULE_SOURCES
	i_layout.cc
	i_layoutitem.cc
	inituibase.cc
	odgraphicsitem.cc
	odwindow.cc
	qptr.cc
	uiaction.cc
	uibutton.cc
	uibuttongroup.cc
	uiclipboard.cc
	uicolor.cc
	uicombobox.cc
	uicursor.cc
	uidate.cc
	uidesktopservices.cc
	uidial.cc
	uidialog.cc
	uidockwin.cc
	uieventfilter.cc
	uifiledlg.cc
	uifont.cc
	uigraphicscoltab.cc
	uigraphicsitem.cc
	uigraphicsitemimpl.cc
	uigraphicsscene.cc
	uigraphicsviewbase.cc
	uigroup.cc
	uihelpview.cc
	uiicon.cc
	uilabel.cc
	uilayout.cc
	uilineedit.cc
	uilistbox.cc
	uimain.cc
	uimainwin.cc
	uimdiarea.cc
	uimenu.cc
	uimouseeventblockerbygesture.cc
	uimsg.cc
	uiobj.cc
	uiobjbody.cc
	uiobjectstack.cc
	uipixmap.cc
	uiplfspecific.cc
	uiprogressbar.cc
	uirgbarray.cc
	uirubberband.cc
	uiscrollarea.cc
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
	uitableview.cc
	uitabstack.cc
	uitextedit.cc
	uitoolbar.cc
	uitraydialog.cc
	uitreeview.cc
	uivirtualkeyboard.cc
)

if ( APPLE )
    list ( APPEND OD_MODULE_SOURCES uimacinit.cc )
endif()

SET(OD_MODULE_GUI_PROGS od_DispMsg.cc  )

OD_INIT_MODULE()
