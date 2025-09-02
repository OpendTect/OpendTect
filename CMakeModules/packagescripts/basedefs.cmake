#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#
# CMake script to define base package variables
#

#OpendTect modules and plugins
set( LIBLIST ${OD_CORE_MODULES} )

set( PLUGINS ${OD_CORE_PLUGINS} )
set( EXCLUDE_PLUGINS Hello Tut )
list( REMOVE_ITEM PLUGINS ${EXCLUDE_PLUGINS} )

set( EXECLIST ${OD_CORE_EXECUTABLES} )
set( EXCLUDE_EXECS tut_process_batch tut_simple_prog )
list( REMOVE_ITEM EXECLIST ${EXCLUDE_EXECS} )

set( SPECMODS ${OD_CORE_SPECMODS} )

foreach( TRGT ${OD_THIRDPARTY_TARGETS} )
    if ( "${TRGT}" MATCHES "^Qt.*::" )
        if ( "${TRGT}" MATCHES "^Qt.*::Core$" OR
             "${TRGT}" MATCHES "^Qt.*::DBus$" OR
             "${TRGT}" MATCHES "^Qt.*::Network$" OR
             "${TRGT}" MATCHES "^Qt.*::Sql$" )
            list( APPEND THIRDPARTY_TARGETS ${TRGT} )
        endif()
    elseif ( NOT "${TRGT}" MATCHES "^osg::" AND
             NOT "${TRGT}" MATCHES "^Fontconfig::" AND
             NOT "${TRGT}" MATCHES "^Freetype::" AND
             NOT "${TRGT}" MATCHES "^PNG::" )
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
        if ( "${LIB}" MATCHES "Qt${REGEXESTR1}Core${REGEXESTR2}" OR
             "${LIB}" MATCHES "Qt${REGEXESTR1}DBus${REGEXESTR2}" OR
             "${LIB}" MATCHES "Qt${REGEXESTR1}Network${REGEXESTR2}" OR
             "${LIB}" MATCHES "Qt${REGEXESTR1}Sql${REGEXESTR2}" )
            list( APPEND THIRDPARTY_LIBS ${LIB} )
        endif()
    else()
        if ( NOT "${LIB}" MATCHES "^${SHLIB_PREFIX}osg.*${SHLIB_EXTENSION}.*" AND
             NOT "${LIB}" MATCHES ".*OpenThreads.*${SHLIB_EXTENSION}.*" AND
             NOT "${LIB}" MATCHES ".*${SHLIB_PREFIX}fontconfig.*${SHLIB_EXTENSION}" AND
             NOT "${LIB}" MATCHES ".*${SHLIB_PREFIX}freetype.*${SHLIB_EXTENSION}" AND
             NOT "${LIB}" MATCHES ".*${SHLIB_PREFIX}png.*${SHLIB_EXTENSION}" AND
             NOT "${LIB}" MATCHES ".*${SHLIB_PREFIX}cups.*${SHLIB_EXTENSION}" AND
             NOT "${LIB}" MATCHES "^d3dcompiler" AND
             NOT "${LIB}" MATCHES "^opengl32sw" )
            list( APPEND THIRDPARTY_LIBS ${LIB} )
        endif()
    endif()
endforeach()

unset( REGEXESTR1 )
unset( REGEXESTR2 )

foreach( OD_QTPLUGIN ${OD_QTPLUGINS} )
    if ( "${OD_QTPLUGIN}" MATCHES "^sqldrivers" OR
	 "${OD_QTPLUGIN}" MATCHES "^tls" )
	list( APPEND QTPLUGINS "${OD_QTPLUGIN}" )
    endif()
endforeach()

#No need to include shell scripts in windows base package.
if( WIN32 )
    set( ODSCRIPTS od_external.bat )
else()
    file( GLOB ODSCRIPTS RELATIVE "${CMAKE_INSTALL_PREFIX}/bin"
	    "${CMAKE_INSTALL_PREFIX}/bin/od_*" )
    list( REMOVE_ITEM ODSCRIPTS od_external.bat od_main_debug.bat )
    list( APPEND ODSCRIPTS init_dtect_GL mksethdir )
    if ( APPLE )
	list( APPEND ODSCRIPTS macterm.in )
    endif()
    set( SPECFILES .exec_prog .init_dtect .init_dtect_user install .lic_inst_common
		   .lic_start_common mk_datadir .start_dtect setup.od )
    if ( APPLE )
	list( APPEND SPECFILES od.icns )
    endif()
endif()

if ( APPLE )
    list( APPEND OTHERFILES "${COPYFROMDIR}/../Info.plist" )
    list( APPEND OTHERFILESDEST "${COPYTODIR}/../" )
endif()

set( LMUTIL "${COPYFROMSCRIPTSDIR}/${OD_PLFSUBDIR}/lm.dgb/lmutil" )
if ( WIN32 )
    set( LMUTIL "${LMUTIL}.exe" )
endif()
list( APPEND OTHERFILES "${LMUTIL}" )
list( APPEND OTHERFILESDEST "${COPYTOSCRIPTSDIR}/${OD_PLFSUBDIR}/lm.dgb" )

if ( WIN32 )
    set( ML_FW_RULENMS "${COPYFROMDATADIR}/Firewall/od_remoteservice.txt" )
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

if ( MATLABLink IN_LIST PLUGINS )
    set( EXECDIRS MATLAB )
    list( APPEND SPECFILES odinit.matlab )
endif()

set( LEGALOPTS zlib openssl sqlite proj hdf5 )
set( LEGALTRGTS ZLIB::ZLIB OpenSSL::SSL SQLite::SQLite3 PROJ::proj hdf5::hdf5-shared )
foreach( opt trgt IN ZIP_LISTS LEGALOPTS LEGALTRGTS )
    if ( ${trgt} IN_LIST OD_THIRDPARTY_TARGETS )
        list( APPEND LEGALLIST ${opt} )
    endif()
endforeach()

set( PYTHONDIR odpy;odbind;safety )

set( PACK base )
