#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#
# CMake script to define basegui package variables
#

#OpendTect modules and plugins
set( LIBLIST ${OD_GUI_MODULES} )

set( PLUGINS ${OD_GUI_PLUGINS} )
set( EXCLUDE_PLUGINS uiHello uiTut uiDPSDemo uiQtApp
		     uiTutMadagascar uiCrashMe uiCOLOP )
list( REMOVE_ITEM PLUGINS ${EXCLUDE_PLUGINS} )

set( EXECLIST ${OD_GUI_EXECUTABLES} )
set( EXCLUDE_EXECS od_layouttest od_screentest od_osgtestapp
		   od_sysadmmain tut_app )
list( REMOVE_ITEM EXECLIST ${EXCLUDE_EXECS} )
if ( WIN32 )
    list( APPEND EXECLIST "od_main_debug.bat" )
endif()

set( SPECMODS ${OD_GUI_SPECMODS} )

if ( APPLE )
    list( APPEND SPECFILES od.icns )
endif()

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

foreach( OD_QTPLUGIN ${OD_QTPLUGINS} )
    if ( NOT "${OD_QTPLUGIN}" MATCHES "^sqldrivers" AND
	 NOT "${OD_QTPLUGIN}" MATCHES "^tls" )
        list( APPEND QTPLUGINS "${OD_QTPLUGIN}" )
    endif()
endforeach()

if ( WIN32 )
    set( ML_FW_RULENMS "${COPYFROMDATADIR}/Firewall/od_main.txt"
		"${COPYFROMDATADIR}/Firewall/od_SeisMMBatch.txt" )
    foreach( ML_FW_RULENM ${ML_FW_RULENMS} )
	list( APPEND OTHERFILES "${ML_FW_RULENM}" )
	list( APPEND OTHERFILESDEST "${COPYTODATADIR}/Firewall" )
    endforeach()
endif()

set( DATADIRNMS FileSelProviders SeisTrcTranslators SurveyProviders )
if ( UNIX AND NOT APPLE )
    list( APPEND DATADIRNMS OpenSSL )
endif()
foreach( MOD IN LISTS LIBLIST PLUGINS )
    foreach( DIRNM ${DATADIRNMS} )
	if ( EXISTS "${COPYFROMDATADIR}/${DIRNM}/${MOD}.txt" )
	    list( APPEND OTHERFILES "${COPYFROMDATADIR}/${DIRNM}/${MOD}.txt" )
	    list( APPEND OTHERFILESDEST "${COPYTODATADIR}/${DIRNM}" )
	endif()
    endforeach()
endforeach()

set( PACK base )
