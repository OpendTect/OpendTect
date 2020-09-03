#_______________________Pmake___________________________________________________
#
#	Makefile : 	Basic UI-classes
# 	July 1995	Bert
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
)

set(OD_MODULE_SOURCES
	i_layout.cc
	i_layoutitem.cc
	inituibase.cc
	odgraphicsitem.cc
	odwindow.cc
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
	uidialog.cc
	uidockwin.cc
	uieventfilter.cc
	uifiledlg.cc
	uifileselector.cc
	uifileselectiongroup.cc
	uifont.cc
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
	uitablemodel.cc
	uitabstack.cc
	uitextedit.cc
	uitoolbar.cc
	uitreeview.cc
	uivirtualkeyboard.cc
)


set(OD_MODULE_GUI_PROGS od_DispMsg.cc  )
OD_ADD_SOURCE_FILES( uimainicon.xpm )

find_package( Qt5WebEngineWidgets QUIET )
if ( Qt5WebEngineWidgets_FOUND )
    option( USE_QtWebEngine "Build with Qt5 WebEngineWidgets support" ON )
    if ( USE_QtWebEngine )
      
        set(OD_USEQT ${OD_USEQT} Positioning Quick QuickWidgets Qml
			       WebChannel WebEngineCore WebEngineWidgets )
        set(QT_MOC_HEADERS ${QT_MOC_HEADERS}  
	    i_qwebengineview.h
        )
        set(OD_MODULE_SOURCES ${OD_MODULE_SOURCES}
	  uiwebengine.cc
        )
        add_definitions( -D__withqtwebengine__ )

        install( DIRECTORY ${QTDIR}/resources
	         DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}
	         CONFIGURATIONS ${CMAKE_BUILD_TYPE}
	         USE_SOURCE_PERMISSIONS )
	install( DIRECTORY ${QTDIR}/translations
                 DESTINATION ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}
                 CONFIGURATIONS ${CMAKE_BUILD_TYPE}
                 USE_SOURCE_PERMISSIONS
                 FILES_MATCHING
                 PATTERN "qtwebengine_*.qm"
                 PATTERN "qtwebengine_locales/*.pak" )
	if ( WIN32 )
	    set( QTPOSTFIX "" )
	    if ( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
		set( QTPOSTFIX "d" )
	    endif()
	    OD_INSTALL_LIBRARY( ${QTDIR}/bin/QtWebEngineProcess${QTPOSTFIX}.exe ${CMAKE_BUILD_TYPE} )
	    OD_INSTALL_LIBRARY( ${QTDIR}/bin/qwebengine_convert_dict.exe ${CMAKE_BUILD_TYPE} )
	else()
	    OD_INSTALL_LIBRARY( ${QTDIR}/libexec/QtWebEngineProcess ${CMAKE_BUILD_TYPE} )
	    OD_INSTALL_LIBRARY( ${QTDIR}/bin/qwebengine_convert_dict ${CMAKE_BUILD_TYPE} )
	endif()
    endif( USE_QtWebEngine )
endif( Qt5WebEngineWidgets_FOUND )

OD_INIT_MODULE()
