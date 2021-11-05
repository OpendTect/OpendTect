#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#_______________________________________________________________________________

macro( OD_FIND_QTDIR )
    if ( NOT DEFINED QTDIR )
	if ( Qt${QT_VERSION_MAJOR}_DIR )
	    get_filename_component( QTDIR ${Qt${QT_VERSION_MAJOR}_DIR} DIRECTORY )
	    get_filename_component( QTDIR ${QTDIR} DIRECTORY )
	    get_filename_component( QTDIR ${QTDIR} DIRECTORY )
        else()
	    set( QTDIR "" CACHE PATH "QT Location" )
	    message( FATAL_ERROR "QTDIR is not defined" )
	endif()
    elseif ( WIN32 )
        get_filename_component(QTDIR ${QTDIR} ABSOLUTE)
    endif()
endmacro(OD_FIND_QTDIR)

macro(ADD_TO_LIST_IF_NEW LISTNAME ITEMNAME)
    list( FIND ${LISTNAME} "${ITEMNAME}" ITMINDEX )
    if ( ${ITMINDEX} EQUAL -1 )
	list(APPEND ${LISTNAME} "${ITEMNAME}")
    endif()
endmacro(ADD_TO_LIST_IF_NEW)

macro( QT_INSTALL_PLUGINS )
    if ( APPLE )
	set ( DESTDIR Contents/PlugIns )
    else()
	set ( DESTDIR bin/${OD_PLFSUBDIR}/plugins )
    endif()

    OD_FIND_QTDIR()
    foreach( QTPLUGIN ${QT_REQ_PLUGINS} )
	install( DIRECTORY "${QTDIR}/plugins/${QTPLUGIN}"
	     DESTINATION ${DESTDIR}
	     CONFIGURATIONS Release
	     USE_SOURCE_PERMISSIONS 
	     FILES_MATCHING
	     PATTERN "*.so"
	     PATTERN "*.dll"
	     PATTERN "*.dylib"
	     PATTERN "*d.dll" EXCLUDE
	     PATTERN "*.pdb" EXCLUDE
	     PATTERN "*.so.debug" EXCLUDE
	     PATTERN "*_debug*" EXCLUDE
	     PATTERN "*.dSYM" EXCLUDE )
	install( DIRECTORY "${QTDIR}/plugins/${QTPLUGIN}"
	     DESTINATION ${DESTDIR}
	     CONFIGURATIONS Debug
	     USE_SOURCE_PERMISSIONS 
	     FILES_MATCHING
	     PATTERN "*.so"
	     PATTERN "*.dll"
	     PATTERN "*.dylib" )
    endforeach()

    list( APPEND OD_QTPLUGINS ${QT_REQ_PLUGINS} )
    list( REMOVE_DUPLICATES OD_QTPLUGINS )

    set( OD_QTPLUGINS ${OD_QTPLUGINS} PARENT_SCOPE )
endmacro(QT_INSTALL_PLUGINS)

macro( QT_SETUP_CORE_INTERNALS )
    OD_FIND_QTDIR()
    OD_INSTALL_RESSOURCE( "${QTDIR}/bin/qt.conf" ${CMAKE_BUILD_TYPE} )

    if ( APPLE )
	set ( TRANSCOREDESTDIR Contents/PlugIns )
    else()
	set ( TRANSCOREDESTDIR bin/${OD_PLFSUBDIR} )
    endif()

    install( DIRECTORY "${QTDIR}/translations"
		 DESTINATION ${TRANSCOREDESTDIR}
		 CONFIGURATIONS ${CMAKE_BUILD_TYPE}
		 USE_SOURCE_PERMISSIONS
		 FILES_MATCHING
		 PATTERN "qt_*.qm"
		 PATTERN "qtbase_*.qm" )

    file( GLOB TRANSLATION_FILES  ${QTDIR}/translations/qt_*.qm ${QTDIR}/translations/qtbase_*.qm)
    foreach( TRANSLATIONFILE ${TRANSLATION_FILES} )
	get_filename_component( FILENM ${TRANSLATIONFILE} NAME )
	list( APPEND OD_QT_TRANSLATION_FILES ${FILENM} )
    endforeach()
    if ( UNIX AND NOT APPLE )
	install( CODE "
	    execute_process(
		COMMAND ${CMAKE_COMMAND} -E create_symlink
			${CMAKE_BUILD_TYPE} lib
		WORKING_DIRECTORY
			\"${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}\" ) " )
    endif()

endmacro(QT_SETUP_CORE_INTERNALS)

macro( QT_SETUP_GUI_INTERNALS )

    list( APPEND QT_REQ_PLUGINS
	    iconengines;imageformats;platforms;printsupport )
    if ( WIN32 )
	list( APPEND QT_REQ_PLUGINS styles )
    elseif ( NOT APPLE )
	list( APPEND QT_REQ_PLUGINS
		egldeviceintegrations;platforminputcontexts;xcbglintegrations )
    endif()

    QT_INSTALL_PLUGINS()

endmacro(QT_SETUP_GUI_INTERNALS )

macro( QT_SETUP_WEBENGINE_INTERNALS )

    OD_FIND_QTDIR()
    list( APPEND QT_REQ_PLUGINS bearer;position )
    QT_INSTALL_PLUGINS()

    if ( NOT APPLE )
	install( DIRECTORY ${QTDIR}/resources
		DESTINATION bin/${OD_PLFSUBDIR}
		CONFIGURATIONS ${CMAKE_BUILD_TYPE}
		USE_SOURCE_PERMISSIONS )
    endif()

    if ( APPLE )
	set ( TRANSWEBENGDESTDIR Contents/PlugIns )
    else()
	set ( TRANSWEBENGDESTDIR bin/${OD_PLFSUBDIR} )
    endif()

    install( DIRECTORY ${QTDIR}/translations
	    DESTINATION ${TRANSWEBENGDESTDIR}
	    CONFIGURATIONS ${CMAKE_BUILD_TYPE}
	    USE_SOURCE_PERMISSIONS
	    FILES_MATCHING
	    PATTERN "qtwebengine_*.qm"
	    PATTERN "qtwebengine_locales/*.pak" )
    file( GLOB WEBENGINE_TRANSLATION_FILES  ${QTDIR}/translations/qtwebengine_*.qm ${QTDIR}/translations/qtwebengine_locales/*.pak )

    foreach( WEBTRANSLATIONFILE ${WEBENGINE_TRANSLATION_FILES} )
	get_filename_component( FILENM ${WEBTRANSLATIONFILE} NAME )
	list( APPEND OD_QT_TRANSLATION_FILES ${FILENM} )
    endforeach()
    set( OD_QT_TRANSLATION_FILES ${OD_QT_TRANSLATION_FILES} PARENT_SCOPE )

    if ( APPLE )
	#TODO
    elseif ( WIN32 )
	set( QTPOSTFIX "" )
	if ( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
	    set( QTPOSTFIX "d" )
	endif()
	OD_INSTALL_PROGRAM( "${QTDIR}/bin/QtWebEngineProcess${QTPOSTFIX}.exe" )
    else()
	install( PROGRAMS "${QTDIR}/libexec/QtWebEngineProcess"
		 DESTINATION bin/${OD_PLFSUBDIR}/libexec )
    endif()

    if ( NOT APPLE )
	list( APPEND OD_QTPLUGINS resources )
    endif()

    set( OD_QTPLUGINS ${OD_QTPLUGINS} PARENT_SCOPE )
endmacro(QT_SETUP_WEBENGINE_INTERNALS)

macro( QT_SETUP_XCBQPA_EXTERNALS )
    add_fontconfig( OD_MODULE_EXTERNAL_RUNTIME_LIBS "${OD_MODULE_EXTERNAL_RUNTIME_LIBS}" )
endmacro(QT_SETUP_XCBQPA_EXTERNALS)

macro( QT_SETUP_CORE_EXTERNALS )
    if ( UNIX AND NOT APPLE )
        OD_FIND_QTDIR()
	set( ICU_VERSION_MAJOR "56" )
	set( LIBNMS i18n data uc )
	foreach( LIBNM ${LIBNMS} )
	    set( FILENM "${QTDIR}/lib/libicu${LIBNM}.so.${ICU_VERSION_MAJOR}" )
	    if ( EXISTS "${FILENM}" )
		list ( APPEND OD_MODULE_EXTERNAL_RUNTIME_LIBS "${FILENM}" )
	    endif()
	endforeach()
	#TODO: add libm, libpcre, libz ??
    endif()
endmacro(QT_SETUP_CORE_EXTERNALS)

macro( QT_SETUP_GUI_EXTERNALS )
    if ( WIN32 )
	OD_FIND_QTDIR()
	list( APPEND OD_MODULE_EXTERNAL_RUNTIME_LIBS
		${Qt5Gui_EGL_LIBRARIES}
		${Qt5Gui_OPENGL_LIBRARIES}
		"${QTDIR}/bin/d3dcompiler_47.dll"
		"${QTDIR}/bin/opengl32sw.dll" )
    elseif ( NOT APPLE )
	QT_SETUP_XCBQPA_EXTERNALS()
    endif()
endmacro(QT_SETUP_GUI_EXTERNALS)

macro( OD_ADD_QT )
    if ( OD_NO_QT )
	add_definitions( -DOD_NO_QT )
    else()
	OD_FIND_QTDIR()

	list ( APPEND CMAKE_PREFIX_PATH "${QTDIR}" )
	find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
	find_package( Qt${QT_VERSION_MAJOR}
		COMPONENTS Core LinguistTools
	       	PATHS "${QTDIR}"
	       	NO_DEFAULT_PATH )

	if ( NOT Qt${QT_VERSION_MAJOR}_FOUND )
	    message( FATAL_ERROR "Cannot find/use the Qt5 installation" )
	endif()

	if ( DEFINED CACHE{USE_QtWebEngine} )
	    unset( USE_QtWebEngine )
	    set( USE_QtWebEngine $CACHE{USE_QtWebEngine} CACHE BOOL "Build with Qt5 WebEngineWidgets" FORCE )
	else()
	    option( USE_QtWebEngine "Build with Qt5 WebEngineWidgets" OFF )
	    set( Dtect_QtWebEngine True )
	endif()
	if ( WIN32 )
	     cmake_policy( SET CMP0020 NEW )
	endif()
	QT_SETUP_CORE_INTERNALS()

	unset( QTDIR CACHE )

    endif(OD_NO_QT)

endmacro(OD_ADD_QT)

macro( QT_DTECT_WEBENGINE )
    if ( (DEFINED Dtect_QtWebEngine OR "${USE_QtWebEngine}") OR
	 (NOT DEFINED Dtect_QtWebEngine AND DEFINED CACHE{USE_QtWebEngine} AND
	  "$CACHE{USE_QtWebEngine}") )
	find_package( Qt5 QUIET COMPONENTS WebEngineWidgets )
	if ( Qt5WebEngineWidgets_FOUND )
	    set( USE_QtWebEngine ON CACHE BOOL "Build with Qt5 WebEngineWidgets" FORCE )
	    add_definitions( -D__withqtwebengine__ )
	endif()
    endif()
endmacro(QT_DTECT_WEBENGINE)

macro( QT_DTECT_CHARTS )
    find_package( Qt5 QUIET COMPONENTS Charts )
endmacro()

macro( OD_ADD_XCBQPAMOD )
    if ( Qt5Core_VERSION_MINOR GREATER 12 )
	list( APPEND GUIINSTMODS XcbQpa )
    endif()
    list ( APPEND OD_INSTQT ${GUIINSTMODS} )
endmacro(OD_ADD_XCBQPAMOD)

macro( OD_ADD_QTGUIMOD )
    list ( APPEND GUIINSTMODS Svg )
    if ( UNIX AND NOT APPLE )
	OD_ADD_XCBQPAMOD()
    endif()
    if ( APPLE )
	list ( APPEND GUIINSTMODS DBus )
    endif()
    list ( APPEND OD_INSTQT ${GUIINSTMODS} )
endmacro(OD_ADD_QTGUIMOD)

macro( OD_ADD_QTQUICKMOD )
    if ( Qt5Core_VERSION_MINOR GREATER_EQUAL 2 )
	list ( APPEND OD_INSTQT QuickWidgets )
    endif()
endmacro(OD_ADD_QTQUICKMOD)

macro( OD_ADD_QTMODS )

    set( LINKMODS "" )
    if ( NOT "${OD_USEQT}" STREQUAL "" )
	find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
	find_package( Qt${QT_VERSION_MAJOR} REQUIRED ${OD_USEQT} )

	foreach( QTMOD ${OD_USEQT} )
	    get_link_libraries( DEPLINKMODS Qt::${QTMOD} )
	    list ( APPEND LINKMODS ${DEPLINKMODS} )
	    list ( REMOVE_DUPLICATES LINKMODS )
	    set ( LINKMODS Qt::${QTMOD};${LINKMODS} )
	endforeach()
	if ( NOT "${LINKMODS}" STREQUAL "" )
	    list( REMOVE_DUPLICATES LINKMODS )
	endif()
    endif()

    if ( Qt::Gui IN_LIST LINKMODS )
    	OD_ADD_QTGUIMOD()
    	QT_SETUP_GUI_INTERNALS()
    endif()
    if ( Qt::Quick IN_LIST LINKMODS )
	OD_ADD_QTQUICKMOD()
    endif()
    if ( USE_QtWebEngine AND Qt::WebEngineWidgets IN_LIST LINKMODS )
	QT_SETUP_WEBENGINE_INTERNALS()
    endif()

    set( INSTMODS "" )
    if ( NOT "${OD_INSTQT}" STREQUAL "" )
	find_package(QT NAMES Qt6 Qt5 COMPONENTS Core REQUIRED)
	find_package( Qt${QT_VERSION_MAJOR} REQUIRED ${OD_INSTQT} )

	foreach( QTMOD ${OD_INSTQT} )
	    get_link_libraries( INSTMODS Qt::${QTMOD} )
	    list ( REMOVE_DUPLICATES INSTMODS )
	endforeach()
	foreach( QTMOD ${OD_INSTQT} )
	    set ( INSTMODS Qt::${QTMOD} ${INSTMODS} )
	endforeach()
	if ( NOT "${INSTMODS}" STREQUAL "" )
	    list( REMOVE_DUPLICATES INSTMODS )
	    set( ALLMODS ${INSTMODS} )
	    set( INSTMODS "" )
	    foreach( INSTMOD ${ALLMODS} )
		if ( NOT ${INSTMOD} IN_LIST LINKMODS )
		    list ( APPEND INSTMODS ${INSTMOD} )
		endif()
	    endforeach()
	endif()
    endif()

    if ( LINKMODS )
	get_shared_link_libraries( LINKMODS "${LINKMODS}" )
	list( APPEND OD_ALLQTCOMPS ${LINKMODS} )
    endif()
    if ( INSTMODS )
	get_shared_link_libraries( INSTMODS "${INSTMODS}" )
	list( APPEND OD_ALLQTCOMPS ${INSTMODS} )
    endif()

endmacro( OD_ADD_QTMODS )

macro( OD_SETUP_QT )

    OD_ADD_QTMODS()
    foreach( QTCOMP ${OD_ALLQTCOMPS} )
	if ( NOT TARGET ${QTCOMP} )
	    message( FATAL_ERROR "${QTCOMP} is NOT a valid target" )
	endif()
    endforeach()

    if( QT_MOC_HEADERS )
	set ( OD_MODULE_DIR ${CMAKE_CURRENT_SOURCE_DIR} )
	foreach( HEADER ${QT_MOC_HEADERS} )
	    list( APPEND QT_MOC_INPUT ${OD_MODULE_DIR}/${HEADER} )
	endforeach()
	QT_WRAP_CPP( QT_MOC_OUTFILES ${QT_MOC_INPUT} )
    endif( QT_MOC_HEADERS )

    foreach ( QTMOD ${LINKMODS} )
	get_target_property( INCLUDEDIR ${QTMOD} INTERFACE_INCLUDE_DIRECTORIES )
	list( APPEND OD_MODULE_INCLUDESYSPATH ${INCLUDEDIR} )
    endforeach()

    list( REMOVE_DUPLICATES OD_MODULE_INCLUDESYSPATH )
    if ( LINKMODS )
	list( APPEND OD_MODULE_EXTERNAL_LIBS ${LINKMODS} )
    endif()
    if ( Qt::Core IN_LIST LINKMODS )
	QT_SETUP_CORE_EXTERNALS()
    endif()
    if ( Qt::Gui IN_LIST LINKMODS )
	QT_SETUP_GUI_EXTERNALS()
    endif()
    if ( INSTMODS )
	list( APPEND OD_MODULE_EXTERNAL_RUNTIME_LIBS ${INSTMODS} )
    endif()

    if ( WIN32 )
	set ( CMAKE_CXX_FLAGS "/wd4481 ${CMAKE_CXX_FLAGS}" )
    endif( WIN32 )

endmacro(OD_SETUP_QT)

macro ( SETUP_QT_TRANSLATION POSTFIX )
    if ( QT_LRELEASE_EXECUTABLE AND EXISTS "${QT_LRELEASE_EXECUTABLE}" )
	if ( WIN32 )
	    set ( COMPILE_TRANSLATIONS_EXTENSION cmd )
	else()
	    set ( COMPILE_TRANSLATIONS_EXTENSION sh )
	endif()
	add_custom_target( Compile_Translations_${POSTFIX} EXCLUDE_FROM_ALL
	    ${OpendTect_DIR}/dtect/compile_translations.${COMPILE_TRANSLATIONS_EXTENSION}
	    "${QT_LRELEASE_EXECUTABLE}" ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}
	    VERBATIM
	COMMENT "Compiling translations" )
    
	install(DIRECTORY data/localizations/ DESTINATION ${MISC_INSTALL_PREFIX}/data/localizations
          FILES_MATCHING PATTERN "*.qm")
	
    endif()
endmacro( SETUP_QT_TRANSLATION )
