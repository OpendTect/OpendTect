#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#
# CMake script to define develgui package variables
#

#OpendTect modules and plugins
set( LIBLIST ${OD_GUI_MODULES} )

set( PLUGINS ${OD_GUI_PLUGINS} )
set( EXCLUDE_PLUGINS uiTut uiQtApp uiCrashMe uiCOLOP )
if ( ${PACKAGE_TYPE} STREQUAL "Production" )
    list ( APPEND EXCLUDE_PLUGINS uiHello uiTutMadagascar )
endif()
list( REMOVE_ITEM PLUGINS ${EXCLUDE_PLUGINS} )

set( EXECLIST ${OD_GUI_EXECUTABLES} )
set( EXCLUDE_EXECS od_layouttest od_screentest od_osgtestapp
		   od_sysadmmain tut_app )
list( REMOVE_ITEM EXECLIST ${EXCLUDE_EXECS} )

set( SPECMODS ${OD_GUI_SPECMODS} )

if ( ${PACKAGE_TYPE} STREQUAL "Production" )

    set( COPY_TARGETS FALSE )
    set( APPEND_TO_PACKAGE TRUE )

else()

    foreach( TRGT ${OD_THIRDPARTY_TARGETS} )
	if ( "${TRGT}" MATCHES "^Qt.*::" )
	    if ( NOT "${TRGT}" MATCHES "^Qt.*::Core$" AND
		 NOT "${TRGT}" MATCHES "^Qt.*::DBus$" AND
		 NOT "${TRGT}" MATCHES "^Qt.*::Network$" AND
		 NOT "${TRGT}" MATCHES "^Qt.*::Sql$" )
		list( APPEND THIRDPARTY_TARGETS ${TRGT} )
	    endif()
	elseif ( "${TRGT}" MATCHES "^osg::" OR
		 "${TRGT}" MATCHES "^Fontconfig::" OR
		 "${TRGT}" MATCHES "^Freetype::" OR
		 "${TRGT}" MATCHES "^PNG::" )
	    list( APPEND THIRDPARTY_TARGETS ${TRGT} )
	endif()
    endforeach()

    if ( APPLE )
	set( REGEXESTR2 ".framework$" )
    else()
	set( REGEXESTR1 ".*" )
    endif()

    foreach( LIB ${OD_THIRDPARTY_LIBS} )
	if ( (APPLE AND "${LIB}" MATCHES "^Qt.*${REGEXESTR2}") OR
	     (NOT APPLE AND "${LIB}" MATCHES "^${SHLIB_PREFIX}Qt.*${SHLIB_EXTENSION}.*") )
	    if ( NOT "${LIB}" MATCHES "Qt${REGEXESTR1}Core${REGEXESTR2}" AND
		 NOT "${LIB}" MATCHES "Qt${REGEXESTR1}DBus${REGEXESTR2}" AND
		 NOT "${LIB}" MATCHES "Qt${REGEXESTR1}Network${REGEXESTR2}" AND
		 NOT "${LIB}" MATCHES "Qt${REGEXESTR1}Sql${REGEXESTR2}" )
		list( APPEND THIRDPARTY_LIBS ${LIB} )
	    endif()
	else()
	    if ( "${LIB}" MATCHES "^${SHLIB_PREFIX}osg.*${SHLIB_EXTENSION}.*" OR
		 "${LIB}" MATCHES ".*OpenThreads.*${SHLIB_EXTENSION}.*" OR
		 "${LIB}" MATCHES ".*${SHLIB_PREFIX}fontconfig.*${SHLIB_EXTENSION}" OR
		 "${LIB}" MATCHES ".*${SHLIB_PREFIX}freetype.*${SHLIB_EXTENSION}" OR
		 "${LIB}" MATCHES ".*${SHLIB_PREFIX}png.*${SHLIB_EXTENSION}" OR
		 "${LIB}" MATCHES ".*${SHLIB_PREFIX}cups.*${SHLIB_EXTENSION}" OR
		 "${LIB}" MATCHES "^d3dcompiler" OR
		 "${LIB}" MATCHES "^opengl32sw" )
		list( APPEND THIRDPARTY_LIBS ${LIB} )
	    endif()
	endif()
    endforeach()

    unset( REGEXESTR1 )
    unset( REGEXESTR2 )

    if ( WIN32 )
	unset( QTPLUGINS )
	set( QTPLUGINS_DIR "${CMAKE_INSTALL_PREFIX}/${OD_RUNTIME_DIRECTORY}/../plugins" )
	foreach( QTPLUGIN_FILE ${OD_QTPLUGINS} )
	    if ( NOT "${QTPLUGIN_FILE}" MATCHES "^sqldrivers" AND
		 NOT "${QTPLUGIN_FILE}" MATCHES "^tls" )
		cmake_path( GET QTPLUGIN_FILE PARENT_PATH QTPLUGIN_DIR )
		cmake_path( GET QTPLUGIN_FILE STEM QTPLUGIN_STEM )
		cmake_path( GET QTPLUGIN_FILE EXTENSION QTPLUGIN_FILEEXT )
		if ( EXISTS "${QTPLUGINS_DIR}/${QTPLUGIN_DIR}/${QTPLUGIN_STEM}d${QTPLUGIN_FILEEXT}" )
		    list( APPEND QTPLUGINS "${QTPLUGIN_DIR}/${QTPLUGIN_STEM}d${QTPLUGIN_FILEEXT}" )
		endif()
	    endif()
	endforeach()
	unset( QTPLUGINS_DIR )
    endif()

endif()

set( PACK devel )
