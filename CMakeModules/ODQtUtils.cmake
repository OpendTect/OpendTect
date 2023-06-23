#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

macro( OD_FIND_QTDIR )
    if ( NOT DEFINED QTDIR )
	find_package( QT NAMES Qt6 Qt5 QUIET COMPONENTS Core GLOBAL )
	if ( EXISTS "${QT_DIR}" AND IS_DIRECTORY "${QT_DIR}" )
	    get_filename_component( QTDIR ${QT_DIR} DIRECTORY )
	    get_filename_component( QTDIR ${QTDIR} DIRECTORY )
	    get_filename_component( QTDIR ${QTDIR} DIRECTORY )
        else()
	    unset( QT_DIR CACHE )
	endif()
    elseif ( WIN32 )
	if ( IS_DIRECTORY "${QTDIR}" )
	    get_filename_component(QTDIR ${QTDIR} ABSOLUTE)
	endif()
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
    install( FILES "${QTDIR}/bin/qt.conf"
	     DESTINATION "${OD_RUNTIME_DIRECTORY}" )

    if ( APPLE )
	set ( TRANSCOREDESTDIR Contents/PlugIns )
    else()
	set ( TRANSCOREDESTDIR bin/${OD_PLFSUBDIR} )
    endif()
    install( DIRECTORY "${QTDIR}/translations"
		 DESTINATION ${TRANSCOREDESTDIR}
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
			\"$<IF:$<CONFIG:Debug>,Debug,Release>\" lib
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
		 USE_SOURCE_PERMISSIONS )
    endif()

    if ( APPLE )
	set ( TRANSWEBENGDESTDIR Contents/PlugIns )
    else()
	set ( TRANSWEBENGDESTDIR bin/${OD_PLFSUBDIR} )
    endif()

    install( DIRECTORY ${QTDIR}/translations
	    DESTINATION ${TRANSWEBENGDESTDIR}
	    USE_SOURCE_PERMISSIONS
	    FILES_MATCHING
	    PATTERN "qtwebengine_*.qm"
	    PATTERN "qtwebengine_locales/*.pak" )
    file( GLOB WEBENGINE_TRANSLATION_FILES
		${QTDIR}/translations/qtwebengine_*.qm
		${QTDIR}/translations/qtwebengine_locales/*.pak )

    foreach( WEBTRANSLATIONFILE ${WEBENGINE_TRANSLATION_FILES} )
	get_filename_component( FILENM ${WEBTRANSLATIONFILE} NAME )
	list( APPEND OD_QT_TRANSLATION_FILES ${FILENM} )
    endforeach()
    set( OD_QT_TRANSLATION_FILES ${OD_QT_TRANSLATION_FILES} PARENT_SCOPE )

    if ( WIN32 )
	install( PROGRAMS "${QTDIR}/bin/QtWebEngineProcess$<$<CONFIG:Debug>:d>.exe"
		 DESTINATION "${OD_RUNTIME_DIRECTORY}" )
    elseif ( APPLE )
	#TODO: Does it exist at all?
    else()
	install( PROGRAMS "${QTDIR}/libexec/QtWebEngineProcess"
		 DESTINATION bin/${OD_PLFSUBDIR}/libexec )
    endif()

    if ( NOT APPLE )
	list( APPEND OD_QTPLUGINS resources )
    endif()

    set( OD_QTPLUGINS ${OD_QTPLUGINS} PARENT_SCOPE )
endmacro(QT_SETUP_WEBENGINE_INTERNALS)

macro( QT_SETUP_CORE_EXTERNALS )
    if ( UNIX AND NOT APPLE )
        OD_FIND_QTDIR()
	set( ICU_VERSION_MAJOR "56" )
	set( LIBNMS i18n data uc )
	foreach( LIBNM ${LIBNMS} )
	    set( FILENM "${QTDIR}/lib/libicu${LIBNM}.so.${ICU_VERSION_MAJOR}" )
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
    OD_FIND_QTDIR()
    get_target_property( QT_GUI_IMPORTED_OBJECTS Qt${QT_VERSION_MAJOR}::Gui
			 IMPORTED_OBJECTS )
    if ( WIN32 )
	get_target_property( QT_GUI_CONFIGS Qt${QT_VERSION_MAJOR}::Gui
			     IMPORTED_CONFIGURATIONS )
	foreach( config ${QT_GUI_CONFIGS} )
	    get_target_property( QT_GUI_LINKDEPS_${config} Qt${QT_VERSION_MAJOR}::Gui
				 IMPORTED_LINK_DEPENDENT_LIBRARIES_${config} )
	    if ( QT_GUI_LINKDEPS_${config} )
		foreach( QT_GUI_LINKDEP_${config} ${QT_GUI_LINKDEPS_${config}} )
		    get_target_property( QT_GUI_LINKDEP_LOCATION_${config} ${QT_GUI_LINKDEP_${config}}
					 IMPORTED_LOCATION_${config} )
		    if ( QT_GUI_LINKDEP_LOCATION_${config} )
			list( APPEND QT_GUI_IMPORTED_OBJECTS_${config} "${QT_GUI_LINKDEP_LOCATION_${config}}" )
		    endif()
		endforeach()
		if ( NOT "${QT_GUI_IMPORTED_OBJECTS_${config}}" STREQUAL "" )
		    set_target_properties( Qt${QT_VERSION_MAJOR}::Gui PROPERTIES
			IMPORTED_OBJECTS_${config} "${QT_GUI_IMPORTED_OBJECTS_${config}}" )
		endif()
		unset( QT_GUI_IMPORTED_OBJECTS_${config} )
	    endif()
	    unset( QT_GUI_LINKDEPS_${config} )
	endforeach()
	unset( QT_GUI_CONFIGS )
	set( QT_GUI_ADDS "${QTDIR}/bin/d3dcompiler_47.dll" "${QTDIR}/bin/opengl32sw.dll" )
    elseif ( UNIX AND NOT APPLE )
	#Only because of XcbQpa
	add_fontconfig( QT_GUI_ADDS )
    endif()

    if ( QT_GUI_ADDS )
	if ( QT_GUI_IMPORTED_OBJECTS )
	    list( APPEND QT_GUI_IMPORTED_OBJECTS ${QT_GUI_ADDS} )
	else()
	    set( QT_GUI_IMPORTED_OBJECTS ${QT_GUI_ADDS} )
	endif()
	unset( QT_GUI_ADDS )
	list( REMOVE_DUPLICATES QT_GUI_IMPORTED_OBJECTS )
	set_target_properties( Qt${QT_VERSION_MAJOR}::Gui PROPERTIES
		    IMPORTED_OBJECTS "${QT_GUI_IMPORTED_OBJECTS}" )
    endif()
    unset( QT_GUI_IMPORTED_OBJECTS )
endmacro(QT_SETUP_GUI_EXTERNALS)

macro( OD_ADD_QT )

    if ( NOT OD_NO_QT )
	OD_FIND_QTDIR()

	list ( APPEND CMAKE_PREFIX_PATH "${QTDIR}" )
	find_package( QT NAMES Qt6 Qt5 QUIET COMPONENTS Core GLOBAL )
	if ( QT_VERSION )
	    find_package( Qt${QT_VERSION_MAJOR} QUIET
			  COMPONENTS Core LinguistTools
			  PATHS "${QTDIR}"
			  NO_DEFAULT_PATH GLOBAL )

	    if ( WIN32 )
		 cmake_policy( SET CMP0020 NEW )
	    endif()
	    QT_SETUP_CORE_INTERNALS()
	    OD_ADD_TRANSLATIONS()
	    unset( QTDIR CACHE )
	else()
	    unset( QT_DIR CACHE )
	    set( QTDIR "" CACHE PATH "QT Location" )
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
    if ( QT_VERSION_MAJOR EQUAL 5 )
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
	list ( APPEND GUIINSTMODS DBus )
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
	find_package( QT NAMES Qt6 Qt5 QUIET COMPONENTS Core GLOBAL )
	if ( QT_VERSION )
	    find_package( Qt${QT_VERSION_MAJOR} QUIET COMPONENTS ${OD_USEQT} GLOBAL )
	    foreach( QTMOD ${OD_USEQT} )
		if ( TARGET Qt${QT_VERSION_MAJOR}::${QTMOD} )
		    get_link_libraries( DEPLINKMODS Qt${QT_VERSION_MAJOR}::${QTMOD} )
		    list ( APPEND LINKMODS ${DEPLINKMODS} )
		    list ( REMOVE_DUPLICATES LINKMODS )
		    set ( LINKMODS Qt${QT_VERSION_MAJOR}::${QTMOD};${LINKMODS} )
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
	if ( Qt${QT_VERSION_MAJOR}::Quick IN_LIST LINKMODS )
	    OD_ADD_QTQUICKMOD()
	endif()
	if ( USE_QtWebEngine AND Qt${QT_VERSION_MAJOR}::WebEngineWidgets IN_LIST LINKMODS )
	    QT_SETUP_WEBENGINE_INTERNALS()
	endif()

	set( INSTMODS "" )
	if ( NOT "${OD_INSTQT}" STREQUAL "" )
	    find_package( Qt${QT_VERSION_MAJOR} QUIET COMPONENTS ${OD_INSTQT} GLOBAL )

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
	    if ( NOT TARGET ${QTCOMP} )
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
	    "${QT_LRELEASE_EXECUTABLE}" ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR}
	    VERBATIM
	COMMENT "Compiling translations" )
	unset( CMAKE_FOLDER )
    
	install( DIRECTORY data/localizations
		 DESTINATION "${MISC_INSTALL_PREFIX}/data/localizations"
		 FILES_MATCHING PATTERN "*.qm")
	
    endif()
endmacro( SETUP_QT_TRANSLATION )
