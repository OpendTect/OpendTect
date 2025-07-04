#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

macro( OD_FIND_QT )
    if ( NOT DEFINED QT_ROOT )
	find_package( QT NAMES Qt6 Qt5 QUIET COMPONENTS Core GLOBAL )
	if ( IS_DIRECTORY "${QT_DIR}" )
	    get_filename_component( QT_ROOT "${QT_DIR}/../../../" ABSOLUTE )
        else()
	    unset( QT_DIR CACHE )
	endif()
    elseif ( WIN32 )
	if ( IS_DIRECTORY "${QT_ROOT}" )
	    get_filename_component( QT_ROOT ${QT_ROOT} ABSOLUTE )
	endif()
    endif()
    if ( IS_DIRECTORY "${QT_ROOT}" AND NOT "${QT_ROOT}" IN_LIST CMAKE_PREFIX_PATH )
	list ( APPEND CMAKE_PREFIX_PATH "${QT_ROOT}" )
    endif()
endmacro(OD_FIND_QT)

macro(ADD_TO_LIST_IF_NEW LISTNAME ITEMNAME)
    list( FIND ${LISTNAME} "${ITEMNAME}" ITMINDEX )
    if ( ${ITMINDEX} EQUAL -1 )
	list(APPEND ${LISTNAME} "${ITEMNAME}")
    endif()
endmacro(ADD_TO_LIST_IF_NEW)

macro( QT_INSTALL_PLUGINS )
    OD_FIND_QT()
    foreach( QTPLUGIN ${QT_REQ_PLUGINS} )
	install( DIRECTORY "${QT_ROOT}/plugins/${QTPLUGIN}"
	     DESTINATION "${OD_RUNTIME_DIRECTORY}/../plugins"
	     CONFIGURATIONS MinSizeRel;RelWithDebInfo;Release
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
	install( DIRECTORY "${QT_ROOT}/plugins/${QTPLUGIN}"
	     DESTINATION "${OD_RUNTIME_DIRECTORY}/../plugins"
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
    OD_FIND_QT()
    set( QTCONFTXT "[Paths]\nPrefix=..\n" )
    if ( APPLE )
	set( QTCONFTXT "${QTCONFTXT}Translations=Contents/translations\n" )
    endif()
    string( REPLACE ":\\" ":/" CMAKE_INSTALL_PREFIX_ed "${CMAKE_INSTALL_PREFIX}" )
    install( CODE "
	     file( WRITE \"${CMAKE_INSTALL_PREFIX_ed}/${OD_RUNTIME_DIRECTORY}/qt.conf\" \"${QTCONFTXT}\" ) " )

    file( GLOB TRANSLATION_FILES ${QT_ROOT}/translations/qt_*.qm
				 ${QT_ROOT}/translations/qtbase_*.qm )
    install( FILES ${TRANSLATION_FILES}
	     DESTINATION "${OD_RUNTIME_DIRECTORY}/../translations" )

    if ( UNIX AND NOT APPLE )
	install( CODE "
	    execute_process(
		COMMAND ${CMAKE_COMMAND} -E create_symlink
			\"$<IF:$<CONFIG:Debug>,Debug,Release>\" lib
		WORKING_DIRECTORY
			\"${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}\" ) " )
    endif()

endmacro(QT_SETUP_CORE_INTERNALS)

macro( QT_SETUP_GUI_INTERNALS )

    list( APPEND QT_REQ_PLUGINS
	    iconengines;imageformats;platforms )
    if ( WIN32 )
	list( APPEND QT_REQ_PLUGINS styles )
    elseif ( NOT APPLE )
	list( APPEND QT_REQ_PLUGINS
		egldeviceintegrations;platforminputcontexts;xcbglintegrations )
    endif()
    if ( QT_VERSION VERSION_GREATER_EQUAL 6 )
	list( APPEND QT_REQ_PLUGINS tls )
    endif()

    QT_INSTALL_PLUGINS()

endmacro(QT_SETUP_GUI_INTERNALS )

macro( QT_SETUP_SQL_INTERNALS )
    list( APPEND QT_REQ_PLUGINS sqldrivers )
    QT_INSTALL_PLUGINS()
endmacro(QT_SETUP_SQL_INTERNALS )

macro( QT_SETUP_QML_INTERNALS )

    if ( QT_VERSION VERSION_GREATER_EQUAL 6.5 )
	qt_policy( SET QTP0001 NEW )
    endif()

endmacro( QT_SETUP_QML_INTERNALS )

macro( QT_SETUP_PRINTSUPPORT_INTERNALS )

    if ( QT_VERSION VERSION_LESS 6 OR (NOT APPLE AND NOT WIN32) )
	list( APPEND QT_REQ_PLUGINS printsupport )
	QT_INSTALL_PLUGINS()
    endif()

    if ( APPLE AND QT_VERSION VERSION_GREATER_EQUAL 6 )
	if ( TARGET Cups::Cups )
	    set_target_properties( Qt${QT_VERSION_MAJOR}::PrintSupport PROPERTIES
			IMPORTED_LINK_DEPENDENT_LIBRARIES Cups::Cups )
	    string( CONCAT CTEST_CUSTOM_WARNING_EXCEPTION "${CTEST_CUSTOM_WARNING_EXCEPTION}"
		" \"warning: dylib.*libcups\\\\.dylib.*was built for newer macOS version.*than being linked\"" )
	    set( CTEST_CUSTOM_WARNING_EXCEPTION ${CTEST_CUSTOM_WARNING_EXCEPTION} PARENT_SCOPE )
	else()
	    message( SEND_ERROR "Cannot use the Qt installation: Qt::PrintSupport requires cups to be installed\nPlease set Cups_ROOT to the location of the cups installation\cups may be installed with brew" )
	endif()
    endif()

endmacro(QT_SETUP_PRINTSUPPORT_INTERNALS)

macro( QT_SETUP_WEBENGINE_INTERNALS )

    OD_FIND_QT()
    list( APPEND QT_REQ_PLUGINS position )
    if ( QT_VERSION VERSION_LESS 6 )
	list( APPEND QT_REQ_PLUGINS bearer )
    endif()
    QT_INSTALL_PLUGINS()

    file( GLOB WEBENGINE_TRANSLATION_FILES "${QT_ROOT}/translations/qtwebengine_*.qm" )
    install( FILES ${WEBENGINE_TRANSLATION_FILES}
	     DESTINATION "${OD_RUNTIME_DIRECTORY}/../translations" )

    if ( APPLE )
	set( WEBENGINE_RESOURCES_DIR "${QT_ROOT}/lib/QtWebEngineCore.framework/Resources" )
	set( WEBENGINE_LOCALES_DIR "${WEBENGINE_RESOURCES_DIR}" )
    else()
	set( WEBENGINE_RESOURCES_DIR "${QT_ROOT}/resources" )
	set( WEBENGINE_LOCALES_DIR "${QT_ROOT}/translations" )
    endif()

    install( DIRECTORY "${WEBENGINE_LOCALES_DIR}/qtwebengine_locales"
	     DESTINATION "${OD_RUNTIME_DIRECTORY}/../translations" )

    file( GLOB WEBENGINE_RESOURCES_FILES
		"${WEBENGINE_RESOURCES_DIR}/icudtl.dat"
		"${WEBENGINE_RESOURCES_DIR}/qtwebengine_*.pak" )
    if ( QT_VERSION VERSION_GREATER_EQUAL 6.6 )
	if ( WIN32 )
	    list( APPEND WEBENGINE_RESOURCES_FILES
		    "${WEBENGINE_RESOURCES_DIR}/v8_context_snapshot$<$<CONFIG:Debug>:.debug>.bin" )
	elseif ( APPLE )
	    list( APPEND WEBENGINE_RESOURCES_FILES
		    "${WEBENGINE_RESOURCES_DIR}/v8_context_snapshot.arm64.bin"
		    "${WEBENGINE_RESOURCES_DIR}/v8_context_snapshot.x86_64.bin" )
	else()
	    list( APPEND WEBENGINE_RESOURCES_FILES
		    "${WEBENGINE_RESOURCES_DIR}/v8_context_snapshot.bin" )
	endif()
    endif()
    install( FILES ${WEBENGINE_RESOURCES_FILES}
	     DESTINATION "${OD_RUNTIME_DIRECTORY}/../resources" )

    if ( WIN32 )
	install( PROGRAMS "${QT_ROOT}/bin/QtWebEngineProcess$<$<CONFIG:Debug>:d>.exe"
		 DESTINATION "${OD_RUNTIME_DIRECTORY}" )
    elseif ( APPLE )
	install( DIRECTORY "${QT_ROOT}/lib/QtWebEngineCore.framework/Helpers/QtWebEngineProcess.app"
		 DESTINATION "${OD_RUNTIME_DIRECTORY}/../Helpers" )
    else()
	install( PROGRAMS "${QT_ROOT}/libexec/QtWebEngineProcess"
		 DESTINATION "${OD_RUNTIME_DIRECTORY}/../libexec" )
    endif()
    # include qwebengine_convert_dict ?

endmacro(QT_SETUP_WEBENGINE_INTERNALS)

macro( QT_SETUP_CORE_EXTERNALS )
    if ( UNIX AND NOT APPLE )
        OD_FIND_QT()
	if ( QT_VERSION VERSION_GREATER_EQUAL 6.7 )
	    set( ICU_VERSION_MAJOR "73" )
	else()
	    set( ICU_VERSION_MAJOR "56" )
	endif()
	set( LIBNMS i18n data uc )
	foreach( LIBNM ${LIBNMS} )
	    set( FILENM "${QT_ROOT}/lib/libicu${LIBNM}.so.${ICU_VERSION_MAJOR}" )
	    if ( EXISTS "${FILENM}" )
		list ( APPEND QT_CORE_ICU_OBJECTS "${FILENM}" )
	    endif()
	endforeach()
	#TODO: add libm, libpcre, libz ??
	get_target_property( QT_CORE_IMPORTED_OBJECTS Qt${QT_VERSION_MAJOR}::Core
			     IMPORTED_OBJECTS )
	if ( QT_CORE_IMPORTED_OBJECTS )
	    list( APPEND QT_CORE_IMPORTED_OBJECTS "${QT_CORE_ICU_OBJECTS}" )
	    list( REMOVE_DUPLICATES QT_CORE_IMPORTED_OBJECTS )
	else()
	    set( QT_CORE_IMPORTED_OBJECTS "${QT_CORE_ICU_OBJECTS}" )
	endif()
	set_target_properties( Qt${QT_VERSION_MAJOR}::Core PROPERTIES
		IMPORTED_OBJECTS "${QT_CORE_IMPORTED_OBJECTS}" )
	unset( QT_CORE_ICU_OBJECTS )
	unset( QT_CORE_IMPORTED_OBJECTS )
    endif()
endmacro(QT_SETUP_CORE_EXTERNALS)

macro( QT_SETUP_GUI_EXTERNALS )
    OD_FIND_QT()
    if ( WIN32 )
	set( QT_GUI_ADDS "${QT_ROOT}/bin/d3dcompiler_47.dll" "${QT_ROOT}/bin/opengl32sw.dll" )
	get_target_property( QT_GUI_IMPORTED_OBJECTS Qt${QT_VERSION_MAJOR}::Gui
			     IMPORTED_OBJECTS )
	if ( QT_GUI_IMPORTED_OBJECTS )
	    list( APPEND QT_GUI_IMPORTED_OBJECTS ${QT_GUI_ADDS} )
	else()
	    set( QT_GUI_IMPORTED_OBJECTS ${QT_GUI_ADDS} )
	endif()
	unset( QT_GUI_ADDS )
	list( REMOVE_DUPLICATES QT_GUI_IMPORTED_OBJECTS )
	set_target_properties( Qt${QT_VERSION_MAJOR}::Gui PROPERTIES
		    IMPORTED_OBJECTS "${QT_GUI_IMPORTED_OBJECTS}" )
	unset( QT_GUI_IMPORTED_OBJECTS )
    elseif( UNIX AND NOT APPLE )
	#Only because of XcbQpa
	add_fontconfig( QT_GUI_ADDS )
	set_target_properties( Qt${QT_VERSION_MAJOR}::Gui PROPERTIES
	    IMPORTED_LINK_DEPENDENT_LIBRARIES "${QT_GUI_ADDS}" )
	unset( QT_GUI_ADDS )
    endif()
endmacro(QT_SETUP_GUI_EXTERNALS)

macro(QT_SETUP_QML_EXTERNALS)
#    qt6_add_qml_module( ${OD_MODULE_NAME}
#	URI ${OD_MODULE_NAME}_qml
#	OUTPUT_DIRECTORY ${OD_MODULE_NAME}_qml
#	VERSION 1.0
#	QML_FILES Main.qml
#    )
endmacro(QT_SETUP_QML_EXTERNALS)

macro( QT_SETUP_PRINTSUPPORT_EXTERNALS )
    OD_FIND_QT()
    if ( APPLE AND QT_VERSION VERSION_GREATER_EQUAL 6 )
	if ( TARGET Cups::Cups )
	    list( APPEND INSTMODS Cups::Cups )
	else()
	    message( SEND_ERROR "Cups not added as runtime dependency" )
	endif()
    endif()
endmacro(QT_SETUP_PRINTSUPPORT_EXTERNALS)

macro( OD_ADD_QT )

    if ( NOT OD_NO_QT )
	OD_FIND_QT()

	find_package( QT NAMES Qt6 Qt5 QUIET COMPONENTS Core GLOBAL )
	if ( QT_VERSION )
	    find_package( Qt${QT_VERSION_MAJOR} QUIET
			  COMPONENTS Core LinguistTools
			  PATHS "${QT_ROOT}"
			  NO_DEFAULT_PATH GLOBAL )

	    if ( WIN32 )
		 cmake_policy( SET CMP0020 NEW )
	    endif()
	    if ( UNIX AND QT_VERSION VERSION_GREATER_EQUAL 6 AND NOT OD_NO_QPRINTSUPPORT )
		OD_FIND_Cups()
	    endif()
	    QT_SETUP_CORE_INTERNALS()
	    OD_ADD_TRANSLATIONS()
	    unset( QT_ROOT CACHE )
	else()
	    unset( QT_DIR CACHE )
	    set( QT_ROOT "" CACHE PATH "QT Location" )
	    message( SEND_ERROR "Cannot find/use the Qt installation" )
	endif()

    endif()

endmacro(OD_ADD_QT)

macro( QT_DTECT_WEBENGINE )
    if ( NOT DEFINED CACHE{USE_QtWebEngine} OR
	 (DEFINED CACHE{USE_QtWebEngine} AND "$CACHE{USE_QtWebEngine}") )
	find_package( Qt${QT_VERSION_MAJOR} QUIET COMPONENTS WebEngineWidgets GLOBAL )
	if ( TARGET Qt${QT_VERSION_MAJOR}::WebEngineWidgets )
	    set( USE_QtWebEngine ON CACHE BOOL "Build with Qt WebEngineWidgets" FORCE )
	    list( APPEND OD_MODULE_COMPILE_DEFINITIONS "__withqtwebengine__" )
	else()
	    set( USE_QtWebEngine OFF CACHE BOOL "Build with Qt WebEngineWidgets" FORCE )
	endif()
    endif()
endmacro(QT_DTECT_WEBENGINE)

macro( QT_DTECT_CHARTS )
    if ( QT_VERSION VERSION_GREATER_EQUAL 6 )
	find_package( Qt${QT_VERSION_MAJOR} QUIET COMPONENTS Charts GLOBAL )
    endif()
endmacro()

macro( OD_ADD_XCBQPAMOD )
    if ( QT_VERSION_MAJOR EQUAL 5 AND Qt5Core_VERSION_MINOR GREATER 12 )
	list ( APPEND GUIINSTMODS XcbQpa )
    elseif ( QT_VERSION_MAJOR GREATER_EQUAL 6 )
	list ( APPEND GUIINSTMODS XcbQpaPrivate )
    endif()
endmacro(OD_ADD_XCBQPAMOD)

macro( OD_ADD_QTGUIMOD )
    list ( APPEND GUIINSTMODS Svg )
    if ( UNIX )
	if ( QT_VERSION_MAJOR EQUAL 5 )
	    list ( APPEND GUIINSTMODS DBus )
	endif()
	if ( NOT APPLE )
	    OD_ADD_XCBQPAMOD()
	endif()
    endif()
    list ( APPEND OD_INSTQT ${GUIINSTMODS} )
endmacro(OD_ADD_QTGUIMOD)

macro( OD_ADD_QTQUICKMOD )
    if ( (QT_VERSION_MAJOR EQUAL 5 AND Qt5Core_VERSION_MINOR GREATER_EQUAL 2) OR
	  QT_VERSION_MAJOR GREATER_EQUAL 6 )
	list ( APPEND OD_INSTQT QuickWidgets )
    endif()
endmacro(OD_ADD_QTQUICKMOD)

macro( OD_ADD_QTMODS )

    set( LINKMODS "" )
    if ( NOT "${OD_USEQT}" STREQUAL "" )
	if ( DEFINED QT_VERSION_MAJOR AND TARGET Qt${QT_VERSION_MAJOR}::Core )
	    foreach( QTMOD IN LISTS OD_USEQT )
		if ( NOT TARGET Qt${QT_VERSION_MAJOR}::${QTMOD} )
		    list( APPEND QT_FIND_MOD ${QTMOD} )
		endif()
	    endforeach()
	    if ( DEFINED QT_FIND_MOD )
		find_package( Qt${QT_VERSION_MAJOR} QUIET COMPONENTS ${QT_FIND_MOD} GLOBAL )
	    endif()
	    unset( QT_FIND_MOD )
	    if ( QT_VERSION_MAJOR GREATER_EQUAL 6 AND UNIX AND NOT APPLE AND
		 TARGET Qt${QT_VERSION_MAJOR}::DBus )
		get_target_property( DBUS_CONFIGS Qt${QT_VERSION_MAJOR}::DBus IMPORTED_CONFIGURATIONS )
		set_target_properties( Qt${QT_VERSION_MAJOR}::DBus PROPERTIES
			IMPORTED_LINK_DEPENDENT_LIBRARIES "" )
		foreach( DBUS_CONFIG ${DBUS_CONFIGS} )
		    set_target_properties( Qt${QT_VERSION_MAJOR}::DBus PROPERTIES
			    IMPORTED_LINK_DEPENDENT_LIBRARIES_${DBUS_CONFIG} "" )
		endforeach()
		unset( DBUS_CONFIGS )
	    endif()
	    foreach( QTMOD ${OD_USEQT} )
		if ( TARGET Qt${QT_VERSION_MAJOR}::${QTMOD} )
		    get_link_libraries( DEPLINKMODS Qt${QT_VERSION_MAJOR}::${QTMOD} )
		    list ( APPEND LINKMODS ${DEPLINKMODS} )
		    list ( REMOVE_DUPLICATES LINKMODS )
		    set ( LINKMODS Qt${QT_VERSION_MAJOR}::${QTMOD};${LINKMODS} )
		else()
		    message( SEND_ERROR "Cannot find/use the Qt installation: some components are missing for Qt${QT_VERSION_MAJOR}::${QTMOD}" )
		    find_package( Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_FIND_MOD} GLOBAL )
		endif()
	    endforeach()
	    if ( NOT "${LINKMODS}" STREQUAL "" )
		list( REMOVE_DUPLICATES LINKMODS )
	    endif()
	endif()
    endif()

    if ( QT_VERSION )
	if ( Qt${QT_VERSION_MAJOR}::Gui IN_LIST LINKMODS )
	    OD_ADD_QTGUIMOD()
	    QT_SETUP_GUI_INTERNALS()
	endif()
	if ( Qt${QT_VERSION_MAJOR}::Sql IN_LIST LINKMODS )
	    QT_SETUP_SQL_INTERNALS()
	endif()
	if ( Qt${QT_VERSION_MAJOR}::Qml IN_LIST LINKMODS )
	    QT_SETUP_QML_INTERNALS()
	endif()
	if ( Qt${QT_VERSION_MAJOR}::PrintSupport IN_LIST LINKMODS )
	    QT_SETUP_PRINTSUPPORT_INTERNALS()
	endif()
	if ( Qt${QT_VERSION_MAJOR}::Quick IN_LIST LINKMODS )
	    OD_ADD_QTQUICKMOD()
	endif()
	if ( USE_QtWebEngine AND Qt${QT_VERSION_MAJOR}::WebEngineWidgets IN_LIST LINKMODS )
	    QT_SETUP_WEBENGINE_INTERNALS()
	endif()

	set( INSTMODS "" )
	if ( NOT "${OD_INSTQT}" STREQUAL "" )
	    foreach( QTMOD IN LISTS OD_INSTQT )
		if ( NOT TARGET Qt${QT_VERSION_MAJOR}::${QTMOD} )
		    list( APPEND QT_FIND_MOD ${QTMOD} )
		endif()
	    endforeach()
	    if ( DEFINED QT_FIND_MOD )
		find_package( Qt${QT_VERSION_MAJOR} QUIET COMPONENTS ${QT_FIND_MOD} GLOBAL )
	    endif()
	    unset( QT_FIND_MOD )

	    foreach( QTMOD ${OD_INSTQT} )
		if ( NOT TARGET Qt${QT_VERSION_MAJOR}::${QTMOD} )
		    find_package( Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${OD_INSTQT} GLOBAL )
		endif()
		get_link_libraries( INSTMODS Qt${QT_VERSION_MAJOR}::${QTMOD} )
		list ( REMOVE_DUPLICATES INSTMODS )
	    endforeach()
	    foreach( QTMOD ${OD_INSTQT} )
		set ( INSTMODS Qt${QT_VERSION_MAJOR}::${QTMOD} ${INSTMODS} )
	    endforeach()
	    if ( NOT "${INSTMODS}" STREQUAL "" )
		list( REMOVE_DUPLICATES INSTMODS )
		set( ALLMODS ${INSTMODS} )
		set( INSTMODS "" )
		foreach( INSTMOD ${ALLMODS} )
		    if ( NOT ${INSTMOD} IN_LIST LINKMODS )
			list( APPEND INSTMODS ${INSTMOD} )
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
    else()
	unset( QT_DIR CACHE )
    endif()

endmacro( OD_ADD_QTMODS )

macro( OD_SETUP_QT )

    if ( OD_NO_QT )
	list( APPEND OD_MODULE_COMPILE_DEFINITIONS "OD_NO_QT" )
    else()
	set( CMAKE_AUTOMOC ON)
	OD_ADD_QTMODS()
	foreach( QTCOMP ${OD_ALLQTCOMPS} )
	    if ( TARGET ${QTCOMP} )
		od_map_configurations( ${QTCOMP} )
	    else()
		message( SEND_ERROR "${QTCOMP} is NOT a valid target" )
	    endif()
	endforeach()

	foreach ( QTMOD ${LINKMODS} )
	    get_target_property( INCLUDEDIR ${QTMOD} INTERFACE_INCLUDE_DIRECTORIES )
	endforeach()

	if ( LINKMODS )
	    list( APPEND OD_MODULE_EXTERNAL_LIBS ${LINKMODS} )
	endif()
	if ( Qt${QT_VERSION_MAJOR}::Core IN_LIST LINKMODS )
	    QT_SETUP_CORE_EXTERNALS()
	endif()
	if ( Qt${QT_VERSION_MAJOR}::Gui IN_LIST LINKMODS )
	    QT_SETUP_GUI_EXTERNALS()
	endif()
	if ( Qt${QT_VERSION_MAJOR}::Qml IN_LIST LINKMODS )
	    QT_SETUP_QML_EXTERNALS()
	endif()
	if ( Qt${QT_VERSION_MAJOR}::PrintSupport IN_LIST LINKMODS )
	    QT_SETUP_PRINTSUPPORT_EXTERNALS()
	endif()
	if ( INSTMODS )
	    list( APPEND OD_MODULE_EXTERNAL_RUNTIME_LIBS ${INSTMODS} )
	endif()

	if ( CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" )
	    list( APPEND OD_MODULE_COMPILE_OPTIONS "/wd4481" )
	endif()
    endif()

endmacro(OD_SETUP_QT)

macro ( SETUP_QT_TRANSLATION POSTFIX )
    if ( QT_LRELEASE_EXECUTABLE AND EXISTS "${QT_LRELEASE_EXECUTABLE}" )
	if ( WIN32 )
	    set ( COMPILE_TRANSLATIONS_EXTENSION cmd )
	else()
	    set ( COMPILE_TRANSLATIONS_EXTENSION sh )
	endif()
	set( CMAKE_FOLDER "Base" )
	add_custom_target( Compile_Translations_${POSTFIX} EXCLUDE_FROM_ALL
	    ${OpendTect_DIR}/dtect/compile_translations.${COMPILE_TRANSLATIONS_EXTENSION}
	    "${QT_LRELEASE_EXECUTABLE}" "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}"
	    VERBATIM
	COMMENT "Compiling translations" )
	unset( CMAKE_FOLDER )

	install( DIRECTORY "${CMAKE_SOURCE_DIR}/share/localizations"
		 DESTINATION "${OD_DATA_INSTALL_RELPATH}/localizations"
		 FILES_MATCHING PATTERN "*.qm" )

    endif()
endmacro( SETUP_QT_TRANSLATION )
