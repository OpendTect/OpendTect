#_______________________Pmake___________________________________________________
#
#	Makefile : 	Basic UI-classes
# 	July 1995	A.H. Bril
#_______________________________________________________________________________

set(OD_MODULE_DEPS Network )
set(OD_USEQT Widgets PrintSupport)
#set(OD_TEST_PROGS uigraphicsview.cc)
set(OD_TEST_PROGS )
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
	i_qtextedit.h
	i_qtoolbar.h
	i_qtreeview.h
	i_qtxtbrowser.h
)

set(OD_MODULE_SOURCES
	i_layout.cc
	i_layoutitem.cc
	inituibase.cc
	odgraphicsitem.cc
	qptr.cc
	uiaction.cc
	uibaseobject.cc
	uibutton.cc
	uibuttongroup.cc
	uiclipboard.cc
	uicolor.cc
	uicombobox.cc
	uicursor.cc
	uidate.cc
	uidesktopservices.cc
	uidial.cc
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
	uivirtualkeyboard.cc
)

if ( APPLE )
    list ( APPEND OD_MODULE_SOURCES uimacinit.cc )
endif()

set(OD_MODULE_GUI_PROGS od_DispMsg.cc  )
OD_ADD_SOURCE_FILES( uimainicon.xpm )
OD_INIT_MODULE()
