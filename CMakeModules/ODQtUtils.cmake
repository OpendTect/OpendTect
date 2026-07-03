#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

macro( OD_FIND_QT )

    if ( NOT QT_VERSION )
	find_package( QT QUIET COMPONENTS Core CONFIG GLOBAL NAMES Qt6 Qt5
			PATHS "${QT_ROOT}"
			HINTS "${CMAKE_PREFIX_PATH}"
			NO_DEFAULT_PATH )
    endif()

endmacro(OD_FIND_QT)

macro( OD_ADD_QT_COMPS COMPS VERBOSITY )
    find_package( Qt${QT_VERSION_MAJOR} ${VERBOSITY}
		    COMPONENTS ${COMPS} CONFIG GLOBAL
		    PATHS "${QT_DIR}" NO_DEFAULT_PATH )
    foreach( COMP ${COMPS} )
	if ( TARGET Qt${QT_VERSION_MAJOR}::${COMP} )
	    od_map_configurations( Qt${QT_VERSION_MAJOR}::${COMP} )
	endif()
    endforeach()
endmacro( OD_ADD_QT_COMPS )

macro( OD_QUERY_QMAKE_INSTALL_DIRS )

    set( _od_qmake_install_keys
	    QT_INSTALL_BINS
	    QT_INSTALL_DATA
	    QT_INSTALL_LIBEXECS
	    QT_INSTALL_LIBS
	    QT_INSTALL_PLUGINS
	    QT_INSTALL_PREFIX
	    QT_INSTALL_TRANSLATIONS )

    set( _need_qmake_query FALSE )
    foreach( _key IN ITEMS ${_od_qmake_install_keys} )
	if ( NOT IS_DIRECTORY "${${_key}}" )
	    set( _need_qmake_query TRUE )
	endif()
    endforeach()

    if ( EXISTS "${QMAKE_EXECUTABLE}" )
	set( _qmake "${QMAKE_EXECUTABLE}" )
    else()
	get_target_property( _qmake Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION )
    endif()
    if ( NOT _qmake )
	set( _need_qmake_query FALSE )
    endif()

    if ( _need_qmake_query )
	execute_process( COMMAND "${_qmake}" -query
	    OUTPUT_VARIABLE _qmake_query
	    RESULT_VARIABLE _qmake_result )

	if ( _qmake_result EQUAL 0 )
	    string( REPLACE "\n" ";" _qmake_lines "${_qmake_query}" )
	    foreach( _line IN LISTS _qmake_lines )
		string( FIND "${_line}" ":" _colon_pos )
		if ( _colon_pos LESS 1 )
		    continue()
		endif()
		string( SUBSTRING "${_line}" 0 ${_colon_pos} _key )
		math( EXPR _val_start "${_colon_pos} + 1" )
		string( SUBSTRING "${_line}" ${_val_start} -1 _val )
		string( STRIP "${_val}" _val )
		if ( NOT "${_key}" IN_LIST _od_qmake_install_keys )
		    continue()
		endif()
		if ( IS_DIRECTORY "${_val}" )
		    set( ${_key} "${_val}" CACHE PATH "Qt ${_key} from qmake -query" FORCE )
		endif()
	    endforeach()
	endif()
    endif()

    if ( NOT IS_DIRECTORY "${QT_INSTALL_TRANSLATIONS}" )
	message( WARNING "Cannot locate Qt translations directory, they might not get installed" )
	if ( _qmake )
	    cmake_path( GET _qmake PARENT_PATH _qmake_parent )
	    set( QT_INSTALL_TRANSLATIONS "${_qmake_parent}/../translations" )
	    cmake_path( NORMAL_PATH QT_INSTALL_TRANSLATIONS )
	endif()
    endif()

    if ( NOT IS_DIRECTORY "${QT_INSTALL_PLUGINS}" )
	message( WARNING "Cannot locate Qt plugins directory, they might not get installed" )
	if ( _qmake )
	    cmake_path( GET _qmake PARENT_PATH _qmake_parent )
	    set( QT_INSTALL_PLUGINS "${_qmake_parent}/../plugins" )
	    cmake_path( NORMAL_PATH QT_INSTALL_PLUGINS )
	endif()
    endif()

    unset( _od_qmake_install_keys )
    unset( _need_qmake_query )
    unset( _qmake )
    unset( _qmake_query )
    unset( _qmake_result )
    unset( _qmake_lines )
    unset( _line )
    unset( _colon_pos )
    unset( _key )
    unset( _val_start )
    unset( _val )
    unset( _qmake_parent )

endmacro( OD_QUERY_QMAKE_INSTALL_DIRS )

macro(ADD_TO_LIST_IF_NEW LISTNAME ITEMNAME)
    list( FIND ${LISTNAME} "${ITEMNAME}" ITMINDEX )
    if ( ${ITMINDEX} EQUAL -1 )
	list(APPEND ${LISTNAME} "${ITEMNAME}")
    endif()
endmacro(ADD_TO_LIST_IF_NEW)

macro( QT_INSTALL_PLUGINS )
    foreach( QTPLUGIN_FILE ${QT_REQ_PLUGINS} )
	if ( NOT EXISTS "${QT_INSTALL_PLUGINS}/${QTPLUGIN_FILE}" )
	    continue()
	endif()
	cmake_path( GET QTPLUGIN_FILE PARENT_PATH QTPLUGIN_DIR )
	if ( WIN32 )
	    cmake_path( GET QTPLUGIN_FILE STEM QTPLUGIN_STEM )
	    cmake_path( GET QTPLUGIN_FILE EXTENSION QTPLUGIN_FILEEXT )
	    install( FILES "${QT_INSTALL_PLUGINS}/${QTPLUGIN_FILE}"
		CONFIGURATIONS MinSizeRel;RelWithDebInfo;Release
		DESTINATION "${OD_RUNTIME_DIRECTORY}/../plugins/${QTPLUGIN_DIR}" )
	    if ( EXISTS "${QT_INSTALL_PLUGINS}/${QTPLUGIN_DIR}/${QTPLUGIN_STEM}.pdb" )
		install( FILES "${QT_INSTALL_PLUGINS}/${QTPLUGIN_DIR}/${QTPLUGIN_STEM}.pdb"
		    CONFIGURATIONS RelWithDebInfo
		    DESTINATION "${OD_RUNTIME_DIRECTORY}/../plugins/${QTPLUGIN_DIR}" )
	    endif()
	    if ( EXISTS "${QT_INSTALL_PLUGINS}/${QTPLUGIN_DIR}/${QTPLUGIN_STEM}d${QTPLUGIN_FILEEXT}" )
		install( FILES "${QT_INSTALL_PLUGINS}/${QTPLUGIN_DIR}/${QTPLUGIN_STEM}d${QTPLUGIN_FILEEXT}"
		    CONFIGURATIONS Debug
		    DESTINATION "${OD_RUNTIME_DIRECTORY}/../plugins/${QTPLUGIN_DIR}" )
	    endif()
	    if ( EXISTS "${QT_INSTALL_PLUGINS}/${QTPLUGIN_DIR}/${QTPLUGIN_STEM}d.pdb" )
		install( FILES "${QT_INSTALL_PLUGINS}/${QTPLUGIN_DIR}/${QTPLUGIN_STEM}d.pdb"
		    CONFIGURATIONS Debug
		    DESTINATION "${OD_RUNTIME_DIRECTORY}/../plugins/${QTPLUGIN_DIR}" )
	    endif()
	else()
	    if ( APPLE )
		install( PROGRAMS "${QT_INSTALL_PLUGINS}/${QTPLUGIN_FILE}"
		    CONFIGURATIONS MinSizeRel;RelWithDebInfo;Release
		    DESTINATION "${OD_RUNTIME_DIRECTORY}/../plugins/${QTPLUGIN_DIR}" )
		install( PROGRAMS "${QT_INSTALL_PLUGINS}/${QTPLUGIN_FILE}"
		    CONFIGURATIONS Debug
		    DESTINATION "${OD_RUNTIME_DIRECTORY}/../../plugins/${QTPLUGIN_DIR}" )
	    else()
		install( PROGRAMS "${QT_INSTALL_PLUGINS}/${QTPLUGIN_FILE}"
		    DESTINATION "${OD_RUNTIME_DIRECTORY}/../plugins/${QTPLUGIN_DIR}" )
	    endif()
	endif()
    endforeach()
    list( APPEND OD_QTPLUGINS ${QT_REQ_PLUGINS} )
    list( REMOVE_DUPLICATES OD_QTPLUGINS )

    set( OD_QTPLUGINS ${OD_QTPLUGINS} PARENT_SCOPE )
endmacro(QT_INSTALL_PLUGINS)

macro( QT_SETUP_CORE_INTERNALS )

    set( QTCONFTXT "[Paths]\nPrefix=..\n" )
    string( REPLACE ":\\" ":/" CMAKE_INSTALL_PREFIX_ed "${CMAKE_INSTALL_PREFIX}" )
    if ( APPLE )
	set( QTCONFTXT "${QTCONFTXT}Translations=Contents/translations\n" )
	set( QTCONFTXT_DEBUG "[Paths]\nPrefix=../..\n" )
	set( QTCONFTXT_DEBUG "${QTCONFTXT_DEBUG}Translations=Contents/translations\n" )
	install( CODE "
		    if( \"$<CONFIG>\" STREQUAL \"Debug\" )
		        file( WRITE \"${CMAKE_INSTALL_PREFIX_ed}/${OD_RUNTIME_DIRECTORY}/qt.conf\" \"${QTCONFTXT_DEBUG}\" )
		    else()
		        file( WRITE \"${CMAKE_INSTALL_PREFIX_ed}/${OD_RUNTIME_DIRECTORY}/qt.conf\" \"${QTCONFTXT}\" )
		    endif()
		      " )
    else()
	install( CODE "
		 file( WRITE \"${CMAKE_INSTALL_PREFIX_ed}/${OD_RUNTIME_DIRECTORY}/qt.conf\" \"${QTCONFTXT}\" ) " )
    endif()

    OD_QUERY_QMAKE_INSTALL_DIRS()

    if ( IS_DIRECTORY "${QT_INSTALL_TRANSLATIONS}" )
	file( GLOB TRANSLATION_FILES ${QT_INSTALL_TRANSLATIONS}/qt_*.qm
		${QT_INSTALL_TRANSLATIONS}/qtbase_*.qm )

	if ( APPLE )
	    install( FILES ${TRANSLATION_FILES}
		     DESTINATION "${OD_RUNTIME_DIRECTORY}/..$<$<CONFIG:Debug>:/..>/translations" )
	else()
	    install( FILES ${TRANSLATION_FILES}
		     DESTINATION "${OD_RUNTIME_DIRECTORY}/../translations" )
	endif()
    endif()

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

    list( APPEND QT_REQ_PLUGINS iconengines/${SHLIB_PREFIX}qsvgicon.${SHLIB_EXTENSION}
				imageformats/${SHLIB_PREFIX}qgif.${SHLIB_EXTENSION}
				imageformats/${SHLIB_PREFIX}qico.${SHLIB_EXTENSION}
				imageformats/${SHLIB_PREFIX}qjpeg.${SHLIB_EXTENSION}
				imageformats/${SHLIB_PREFIX}qsvg.${SHLIB_EXTENSION} )
    if ( WIN32 )
	list( APPEND QT_REQ_PLUGINS platforms/qwindows.dll )
    elseif ( APPLE )
	list( APPEND QT_REQ_PLUGINS platforms/libqcocoa.dylib
				    styles/libqmacstyle.dylib )
    else()
	list( APPEND QT_REQ_PLUGINS platforms/libqwayland-egl.so
				    platforms/libqwayland-generic.so
				    platforms/libqxcb.so )
	# For wayland platform
	list( APPEND QT_REQ_PLUGINS
		wayland-decoration-client/libbradient.so
		wayland-graphics-integration-client/libdmabuf-server.so
		wayland-graphics-integration-client/libdrm-egl-server.so
		wayland-graphics-integration-client/libqt-plugin-wayland-egl.so
		wayland-graphics-integration-client/libshm-emulation-server.so
		wayland-graphics-integration-client/libvulkan-server.so
		wayland-shell-integration/libfullscreen-shell-v1.so
		wayland-shell-integration/libivi-shell.so
		wayland-shell-integration/libwl-shell-plugin.so
		wayland-shell-integration/libxdg-shell.so )
	# For xcb platform
	list( APPEND QT_REQ_PLUGINS
		egldeviceintegrations/libqeglfs-emu-integration.so
		egldeviceintegrations/libqeglfs-kms-egldevice-integration.so
		egldeviceintegrations/libqeglfs-x11-integration.so
		platforminputcontexts/libcomposeplatforminputcontextplugin.so
		platforminputcontexts/libibusplatforminputcontextplugin.so
		xcbglintegrations/libqxcb-egl-integration.so
		xcbglintegrations/libqxcb-glx-integration.so )
    endif()
    if ( QT_VERSION VERSION_GREATER_EQUAL 6 )
	list( APPEND QT_REQ_PLUGINS
		tls/${SHLIB_PREFIX}qcertonlybackend.${SHLIB_EXTENSION}
		tls/${SHLIB_PREFIX}qopensslbackend.${SHLIB_EXTENSION} )
	if ( WIN32 )
	    list( APPEND QT_REQ_PLUGINS
			styles/qmodernwindowsstyle.dll
			tls/qschannelbackend.dll )
	elseif ( APPLE )
	    list( APPEND QT_REQ_PLUGINS
	    		tls/libqsecuretransportbackend.dylib )
	else()
	    list( APPEND QT_REQ_PLUGINS
			wayland-decoration-client/libadwaita.so
			wayland-shell-integration/libqt-shell.so )
	    if ( EXISTS "${QT_INSTALL_PLUGINS}/egldeviceintegrations/libqeglfs-kms-integration.so" )
		list( APPEND QT_REQ_PLUGINS
			egldeviceintegrations/libqeglfs-kms-integration.so )
	    endif()
	endif()
	if ( TARGET Qt${QT_VERSION_MAJOR}::QTiffPlugin )
	    list( APPEND QT_REQ_PLUGINS
	    		imageformats/${SHLIB_PREFIX}qicns.${SHLIB_EXTENSION}
			imageformats/${SHLIB_PREFIX}qtga.${SHLIB_EXTENSION}
			imageformats/${SHLIB_PREFIX}qtiff.${SHLIB_EXTENSION}
			imageformats/${SHLIB_PREFIX}qwbmp.${SHLIB_EXTENSION}
			imageformats/${SHLIB_PREFIX}qwebp.${SHLIB_EXTENSION} )
	    if ( APPLE )
		list( APPEND QT_REQ_PLUGINS imageformats/libqmacheif.dylib
					    imageformats/libqmacjp2.dylib )
	    endif()
	else()
	    message( AUTHOR_WARNING "Cannot find the tiff image format plugin: Install the 'Qt Image formats' optional Qt package" )
	endif()
    else()
	list( APPEND QT_REQ_PLUGINS
		imageformats/${SHLIB_PREFIX}qicns.${SHLIB_EXTENSION}
		imageformats/${SHLIB_PREFIX}qtga.${SHLIB_EXTENSION}
		imageformats/${SHLIB_PREFIX}qtiff.${SHLIB_EXTENSION}
		imageformats/${SHLIB_PREFIX}qwbmp.${SHLIB_EXTENSION}
		imageformats/${SHLIB_PREFIX}qwebp.${SHLIB_EXTENSION} )
	if ( WIN32 )
	    list( APPEND QT_REQ_PLUGINS styles/qwindowsvistastyle.dll )
	elseif ( APPLE )
	    list( APPEND QT_REQ_PLUGINS imageformats/libqmacheif.dylib
					imageformats/libqmacjp2.dylib )
	else()
	    list( APPEND QT_REQ_PLUGINS
	    		platforms/libqwayland-xcomposite-egl.so
			platforms/libqwayland-xcomposite-glx.so
			wayland-graphics-integration-client/libxcomposite-egl.so
			wayland-graphics-integration-client/libxcomposite-glx.so
			wayland-shell-integration/libxdg-shell-v5.so
			wayland-shell-integration/libxdg-shell-v6.so )
	endif()
    endif()

    QT_INSTALL_PLUGINS()

endmacro(QT_SETUP_GUI_INTERNALS )

macro( QT_SETUP_SQL_INTERNALS )
    list( APPEND QT_REQ_PLUGINS sqldrivers/${SHLIB_PREFIX}qsqlite.${SHLIB_EXTENSION}
				sqldrivers/${SHLIB_PREFIX}qsqlodbc.${SHLIB_EXTENSION}
				sqldrivers/${SHLIB_PREFIX}qsqlpsql.${SHLIB_EXTENSION} )
    if ( QT_VERSION VERSION_GREATER_EQUAL 6 )
	list( APPEND QT_REQ_PLUGINS sqldrivers/${SHLIB_PREFIX}qsqlmimer.${SHLIB_EXTENSION} )
	if ( WIN32 )
	    if ( QT_VERSION VERSION_GREATER_EQUAL 6.9 )
		list( APPEND QT_REQ_PLUGINS sqldrivers/qsqlibase.dll
					    sqldrivers/qsqloci.dll )
	    endif()
	elseif ( NOT APPLE )
	    list( APPEND QT_REQ_PLUGINS sqldrivers/libqsqlmysql.so )
	endif()
    endif()
    QT_INSTALL_PLUGINS()
endmacro(QT_SETUP_SQL_INTERNALS )

macro( QT_SETUP_QML_INTERNALS )

    if ( QT_VERSION VERSION_GREATER_EQUAL 6.5 )
	qt_policy( SET QTP0001 NEW )
    endif()

endmacro( QT_SETUP_QML_INTERNALS )

macro( QT_SETUP_PRINTSUPPORT_INTERNALS )

    if ( QT_VERSION VERSION_LESS 6 OR (NOT APPLE AND NOT WIN32) )
	if ( WIN32 )
	    list( APPEND QT_REQ_PLUGINS printsupport/windowsprintersupport.dll )
	elseif( APPLE )
	    list( APPEND QT_REQ_PLUGINS printsupport/libcocoaprintersupport.dylib )
	else()
	    list( APPEND QT_REQ_PLUGINS printsupport/libcupsprintersupport.so )
	endif()
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

    list( APPEND QT_REQ_PLUGINS
		position/${SHLIB_PREFIX}qtposition_nmea.${SHLIB_EXTENSION}
		position/${SHLIB_PREFIX}qtposition_positionpoll.${SHLIB_EXTENSION} )
    if ( WIN32 )
	list( APPEND QT_REQ_PLUGINS position/qtposition_winrt.dll )
    elseif( APPLE )
	list( APPEND QT_REQ_PLUGINS position/libqtposition_cl.dylib )
    else()
	list( APPEND QT_REQ_PLUGINS position/libqtposition_geoclue2.so )
    endif()
    if ( QT_VERSION VERSION_LESS 6 )
	list( APPEND QT_REQ_PLUGINS bearer/${SHLIB_PREFIX}qgenericbearer.${SHLIB_EXTENSION} )
	if ( UNIX AND NOT APPLE )
	    list( APPEND QT_REQ_PLUGINS bearer/libqconnmanbearer.so
					bearer/libqnmbearer.so )
	endif()
    endif()
    QT_INSTALL_PLUGINS()

    file( GLOB WEBENGINE_TRANSLATION_FILES "${QT_INSTALL_DATA}/translations/qtwebengine_*.qm" )
    if ( APPLE )
	install( FILES ${WEBENGINE_TRANSLATION_FILES}
		 DESTINATION "${OD_RUNTIME_DIRECTORY}/..$<$<CONFIG:Debug>:/..>/translations" )
    else()
	install( FILES ${WEBENGINE_TRANSLATION_FILES}
		 DESTINATION "${OD_RUNTIME_DIRECTORY}/../translations" )
    endif()

    if ( APPLE )
	set( WEBENGINE_RESOURCES_DIR "${QT_INSTALL_LIBS}/QtWebEngineCore.framework/Resources" )
    else()
	set( WEBENGINE_RESOURCES_DIR "${QT_INSTALL_DATA}/resources" )
    endif()

    if ( APPLE )
	install( DIRECTORY "${QT_INSTALL_DATA}/translations/qtwebengine_locales"
		 DESTINATION "${OD_RUNTIME_DIRECTORY}/..$<$<CONFIG:Debug>:/..>/translations" )
    else()
	install( DIRECTORY "${QT_INSTALL_DATA}/translations/qtwebengine_locales"
		 DESTINATION "${OD_RUNTIME_DIRECTORY}/../translations" )
    endif()

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
	install( PROGRAMS "${QT_INSTALL_LIBEXECS}/QtWebEngineProcess$<$<CONFIG:Debug>:d>.exe"
		 DESTINATION "${OD_RUNTIME_DIRECTORY}" )
    elseif ( APPLE )
	install( DIRECTORY "${QT_INSTALL_LIBS}/QtWebEngineCore.framework/Helpers/QtWebEngineProcess.app"
		 DESTINATION "${OD_RUNTIME_DIRECTORY}/../Helpers" )
    else()
	install( PROGRAMS "${QT_INSTALL_LIBEXECS}/QtWebEngineProcess"
		 DESTINATION "${OD_RUNTIME_DIRECTORY}/../libexec" )
    endif()
    # include qwebengine_convert_dict ?

endmacro(QT_SETUP_WEBENGINE_INTERNALS)

macro( QT_SETUP_CORE_EXTERNALS )
    if ( UNIX AND NOT APPLE )
	if ( QT_VERSION VERSION_GREATER_EQUAL 6.7 )
	    set( ICU_VERSION_MAJOR "73" )
	else()
	    set( ICU_VERSION_MAJOR "56" )
	endif()
	set( LIBNMS i18n data uc )
	foreach( LIBNM ${LIBNMS} )
	    set( FILENM "${QT_INSTALL_LIBS}/libicu${LIBNM}.so.${ICU_VERSION_MAJOR}" )
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
    if ( WIN32 )
	set( D3D_FILE "d3dcompiler_47.dll" )
	set( OPENGL_FILE "opengl32sw.dll" )
	if ( EXISTS "${QT_INSTALL_BINS}/${D3D_FILE}" )
	    list( APPEND QT_GUI_ADDS "${QT_INSTALL_BINS}/${D3D_FILE}" )
	elseif ( EXISTS "$ENV{WINDIR}/System32/${D3D_FILE}" )
	    cmake_path( CONVERT "$ENV{WINDIR}/System32/${D3D_FILE}" TO_CMAKE_PATH_LIST D3D_FILE )
	    list( APPEND QT_GUI_ADDS "${D3D_FILE}" )
	endif()
	if ( EXISTS "${QT_INSTALL_BINS}/${OPENGL_FILE}" )
	    list( APPEND QT_GUI_ADDS "${QT_INSTALL_BINS}/${OPENGL_FILE}" )
	elseif( EXISTS "$ENV{CONDA_PREFIX}/Library/bin/${OPENGL_FILE}" )
	    cmake_path( CONVERT "$ENV{CONDA_PREFIX}/Library/bin/${OPENGL_FILE}" TO_CMAKE_PATH_LIST OPENGL_FILE )
	    list( APPEND QT_GUI_ADDS "${OPENGL_FILE}" )
	endif()
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
	#Only because of XcbQpa and Wayland
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
	if ( QT_VERSION_MAJOR )
	    OD_ADD_QT_COMPS( "Core;LinguistTools" QUIET )
	endif()
	if ( TARGET Qt${QT_VERSION_MAJOR}::Core )
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
	    set( QT_ROOT "" CACHE PATH "QT Location" )
	    message( SEND_ERROR "Cannot find/use the Qt installation" )
	endif()
    endif()

endmacro(OD_ADD_QT)

macro( QT_DTECT_WEBENGINE )
    if ( NOT DEFINED CACHE{USE_QtWebEngine} OR
	 (DEFINED CACHE{USE_QtWebEngine} AND "$CACHE{USE_QtWebEngine}") )
	OD_ADD_QT_COMPS( WebEngineWidgets QUIET )
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
	if ( QT_VERSION VERSION_GREATER_EQUAL 6.10 )
	    OD_ADD_QT_COMPS( Graphs QUIET )
	else()
	    OD_ADD_QT_COMPS( Charts QUIET )
	endif()
    endif()
endmacro()

macro( OD_ADD_XCBQPAMOD )
    if ( QT_VERSION_MAJOR EQUAL 5 AND Qt5Core_VERSION_MINOR GREATER 12 )
	list ( APPEND GUIINSTMODS XcbQpa )
    elseif ( QT_VERSION_MAJOR GREATER_EQUAL 6 )
	list ( APPEND GUIINSTMODS XcbQpaPrivate )
    endif()
endmacro(OD_ADD_XCBQPAMOD)

macro( OD_ADD_WAYLANDQPAMOD )
    list ( APPEND GUIINSTMODS WaylandClient )
    if ( QT_VERSION_MAJOR GREATER_EQUAL 6 )
	set( QT_NO_PRIVATE_MODULE_WARNING ON )
	foreach( WAYLANDEGLMOD WaylandEglClientHwIntegrationPrivate
			       WaylandEglCompositorHwIntegrationPrivate )
	    if ( NOT TARGET Qt${QT_VERSION_MAJOR}::${WAYLANDEGLMOD} )
		OD_ADD_QT_COMPS( "${WAYLANDEGLMOD}" QUIET )
	    endif()
	    if ( TARGET Qt${QT_VERSION_MAJOR}::${WAYLANDEGLMOD} )
		list ( APPEND GUIINSTMODS ${WAYLANDEGLMOD} )
		break()
	    endif()
	endforeach()
	unset( WAYLANDEGLMOD )
    endif()
endmacro( OD_ADD_WAYLANDQPAMOD )

macro( OD_ADD_QTGUIMOD )
    list ( APPEND GUIINSTMODS Svg )
    if ( UNIX )
	if ( QT_VERSION_MAJOR EQUAL 5 )
	    list ( APPEND GUIINSTMODS DBus )
	endif()
	if ( NOT APPLE )
	    OD_ADD_XCBQPAMOD()
	    OD_ADD_WAYLANDQPAMOD()
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

    OD_FIND_QT()
    set( LINKMODS "" )
    if ( NOT "${OD_USEQT}" STREQUAL "" )
	if ( DEFINED QT_VERSION_MAJOR AND TARGET Qt${QT_VERSION_MAJOR}::Core )
	    foreach( QTMOD IN LISTS OD_USEQT )
		if ( NOT TARGET Qt${QT_VERSION_MAJOR}::${QTMOD} )
		    list( APPEND QT_FIND_MOD ${QTMOD} )
		endif()
	    endforeach()
	    if ( DEFINED QT_FIND_MOD )
		OD_ADD_QT_COMPS( "${QT_FIND_MOD}" QUIET )
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
		    OD_ADD_QT_COMPS( "${QT_FIND_MOD}" REQUIRED )
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
		OD_ADD_QT_COMPS( "${QT_FIND_MOD}" QUIET )
	    endif()
	    unset( QT_FIND_MOD )

	    foreach( QTMOD ${OD_INSTQT} )
		if ( NOT TARGET Qt${QT_VERSION_MAJOR}::${QTMOD} )
		    OD_ADD_QT_COMPS( "${QT_FIND_MOD}" REQUIRED )
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
	unset( QT_NO_PRIVATE_MODULE_WARNING )
    endif()

endmacro( OD_ADD_QTMODS )

macro( OD_SETUP_QT )

    if ( OD_NO_QT )
	list( APPEND OD_MODULE_COMPILE_DEFINITIONS "OD_NO_QT" )
    else()
	set( CMAKE_AUTOMOC ON )
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
