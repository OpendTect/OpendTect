#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#
# CMake script with all macros used for the packaging
#

macro( CLEAN_PACK_VARIABLES )
    unset( PACK )
    unset( LIBLIST )
    unset( EXECLIST )
    unset( EXCLUDE_EXECS )
    unset( PLUGINS )
    unset( EXCLUDE_PLUGINS )
    unset( SPECMODS )
    unset( EXCLUDE_SPECMODS )
    unset( COPY_TARGETS )
# root files
    unset( SPECFILES )
# share files
    unset( DATALIST )
    unset( LEGALLIST )
    unset( PYTHONREQLIST )
# bin/script files
# provide file names (only) from the bin folder
    unset( ODSCRIPTS )
    unset( PYTHONDIR )
# any other file or directory
# boths lists OTHERFILES and OTHERFILESDEST must have the same size
# and should use absolute file paths
    unset( OTHERFILES )
    unset( OTHERFILESDEST )
    unset( THIRDPARTY_LIBS )
    unset( THIRDPARTY_TARGETS )
    unset( NO_QTTRANSLATIONS )
    unset( QTPLUGINS )
endmacro(CLEAN_PACK_VARIABLES)

macro( INIT_DESTINATIONDIR PACKAGE_NAME )
    if( "${PACKAGE_NAME}" MATCHES "basedata$" OR "${PACKAGE_NAME}" MATCHES "doc$" )
	set( VER_FILENAME "${PACKAGE_NAME}" )
	if ( APPLE )
	    set ( VER_FILENAME "${VER_FILENAME}_mac" )
	endif()
	set( PACKAGE_FILENAME "${VER_FILENAME}.zip" )
    else()
	set( VER_FILENAME "${PACKAGE_NAME}_${OD_PLFSUBDIR}" )
	set( PACKAGE_FILENAME "${PACKAGE_NAME}_${OD_PLFSUBDIR}.zip" )
    endif()

    if( NOT EXISTS "${PACKAGE_DIR}" )
	file( MAKE_DIRECTORY "${PACKAGE_DIR}" )
    endif()

    set( DESTINATION_DIR "${PACKAGE_DIR}/${REL_DIR}" )
    if( IS_DIRECTORY "${DESTINATION_DIR}" )
	file( REMOVE_RECURSE "${DESTINATION_DIR}" )
    endif()

    if ( APPLE )
	set( MISC_INSTALL_PREFIX "/Contents/Resources" )
    endif()
    set( COPYFROMDIR "${CMAKE_INSTALL_PREFIX}${MISC_INSTALL_PREFIX}" )
    set( COPYTODIR "${DESTINATION_DIR}${MISC_INSTALL_PREFIX}" )
    set( COPYFROMSCRIPTSDIR "${COPYFROMDIR}/bin" )
    set( COPYTOSCRIPTSDIR "${COPYTODIR}/bin" )
    set( COPYFROMDATADIR "${COPYFROMDIR}/share" )
    set( COPYTODATADIR "${COPYTODIR}/share" )
    set( COPYFROMLIBDIR "${CMAKE_INSTALL_PREFIX}/${OD_LIBRARY_DIRECTORY}" )
    set( COPYTOLIBDIR "${DESTINATION_DIR}/${OD_LIBRARY_DIRECTORY}" )
    set( COPYFROMLOCDIR "${CMAKE_INSTALL_PREFIX}/${OD_LOCATION_DIRECTORY}" )
    set( COPYTOLOCDIR "${DESTINATION_DIR}/${OD_LOCATION_DIRECTORY}" )
    set( COPYFROMEXEDIR "${CMAKE_INSTALL_PREFIX}/${OD_RUNTIME_DIRECTORY}" )
    set( COPYTOEXEDIR "${DESTINATION_DIR}/${OD_RUNTIME_DIRECTORY}" )

    if ( EXISTS "${PACKAGE_DIR}/${PACKAGE_FILENAME}" )
	file( REMOVE_RECURSE "${PACKAGE_DIR}/${PACKAGE_FILENAME}" )
    endif()

    file( WRITE "${COPYTODIR}/relinfo/ver.${VER_FILENAME}.txt" ${FULLVER_NAME} )
    file( APPEND "${COPYTODIR}/relinfo/ver.${VER_FILENAME}.txt" "\n" )

    message( STATUS "Preparing package ${VER_FILENAME} ......" )
endmacro(INIT_DESTINATIONDIR)

macro( COPY_LIB )
    if ( APPLE AND IS_DIRECTORY "${COPYFROMLOCDIR}/${LIBNM}" )
	file( COPY "${COPYFROMLOCDIR}/${LIBNM}"
	      DESTINATION "${COPYTOLOCDIR}"
	      FILES_MATCHING
	      PATTERN "*"
	      PATTERN "Headers" EXCLUDE
	      PATTERN "Helpers" EXCLUDE
	      PATTERN "Resources/icudtl.dat" EXCLUDE
	      PATTERN "Resources/*.pak" EXCLUDE
	      PATTERN "Resources/*.prl" EXCLUDE
	      PATTERN "Resources/qtwebengine_locales" EXCLUDE )
    else()
	file( COPY "${COPYFROMLOCDIR}/${LIBNM}"
	      DESTINATION "${COPYTOLOCDIR}"
	      FOLLOW_SYMLINK_CHAIN )
    endif()
endmacro()

macro( COPY_THIRDPARTYLIBS )
    foreach( LIBNM ${THIRDPARTY_LIBS} )
	if ( UNIX AND NOT APPLE )
	    if( "${LIBNM}" MATCHES "^libssl" OR "${LIBNM}" MATCHES "^libcrypto" )
		set( _INST_DIR "OpenSSL" )
	    else()
		get_filename_component( _INST_DIR ${LIBNM} DIRECTORY )
		get_filename_component( LIBNM ${LIBNM} NAME )
	    endif()
	endif()
	if ( "${_INST_DIR}" STREQUAL "" )
	    COPY_LIB()
	else()
	    file( COPY "${COPYFROMLOCDIR}/${_INST_DIR}/${LIBNM}"
		  DESTINATION "${COPYTOLOCDIR}/${_INST_DIR}"
		  FOLLOW_SYMLINK_CHAIN )
	endif()
	unset( _INST_DIR )
    endforeach()

    if ( THIRDPARTY_TARGETS OR QTPLUGINS )
	COPY_QTFILES()
    endif()

endmacro(COPY_THIRDPARTYLIBS)

macro(COPY_QTFILES)

    foreach( TRGT ${THIRDPARTY_TARGETS} )
	if ( "${TRGT}" MATCHES "^Qt.*::Core$" )
	    COPY_QTCORE()
	    break()
	endif()
    endforeach()

    foreach( TRGT ${THIRDPARTY_TARGETS} )
	if ( "${TRGT}" MATCHES "^Qt.*::WebEngineCore$" )
	    COPY_QTWEBENGINE()
	    break()
	endif()
    endforeach()

    foreach( QTPLUGIN_FILE ${QTPLUGINS} )
	cmake_path( GET QTPLUGIN_FILE PARENT_PATH QTPLUGIN_DIR )
	file( COPY "${COPYFROMEXEDIR}/../plugins/${QTPLUGIN_FILE}"
	      DESTINATION "${COPYTOEXEDIR}/../plugins/${QTPLUGIN_DIR}" )
    endforeach()

endmacro(COPY_QTFILES)

macro( COPY_QTCORE )

    file( COPY "${COPYFROMEXEDIR}/qt.conf"
	  DESTINATION "${COPYTOEXEDIR}" )

    if ( "${PACKAGE_TYPE}" STREQUAL "Production" )

	if( UNIX )
	    if ( APPLE )
		file( COPY "${COPYFROMDIR}/qt_menu.nib"
		      DESTINATION "${COPYTODIR}" )
	    else()
		file(COPY "${COPYFROMEXEDIR}/../lib"
		     DESTINATION "${COPYTOEXEDIR}/../" )
	    endif()
	endif()

	if ( NOT NO_QTTRANSLATIONS )
	    file( GLOB TRANSLATION_FILES
		    "${COPYFROMEXEDIR}/../translations/qt_*.qm"
		    "${COPYFROMEXEDIR}/../translations/qtbase_*.qm" )
	    file( COPY ${TRANSLATION_FILES}
		  DESTINATION "${COPYTOEXEDIR}/../translations" )
	    unset( TRANSLATION_FILES )
	endif()

    endif()

endmacro( COPY_QTCORE )

macro( COPY_QTWEBENGINE )

    if ( "${PACKAGE_TYPE}" STREQUAL "Production" )

	if ( NOT NO_QTTRANSLATIONS )
	    file( GLOB WEBENGINE_TRANSLATION_FILES
		    "${COPYFROMEXEDIR}/../translations/qtwebengine_*.qm" )
	    file( COPY ${WEBENGINE_TRANSLATION_FILES}
		  DESTINATION "${COPYTOEXEDIR}/../translations" )
	    unset( WEBENGINE_TRANSLATION_FILES )
	    if ( IS_DIRECTORY "${COPYFROMEXEDIR}/../translations/qtwebengine_locales" )
		file( GLOB WEBENGINE_TRANSLATION_FILES
			"${COPYFROMEXEDIR}/../translations/qtwebengine_locales/*.pak" )
		file( COPY ${WEBENGINE_TRANSLATION_FILES}
		      DESTINATION "${COPYTOEXEDIR}/../translations/qtwebengine_locales" )
		unset( WEBENGINE_TRANSLATION_FILES )
	    endif()

	    file( GLOB WEBENGINE_RESOURCES_FILES
		    "${COPYFROMEXEDIR}/../resources/icudtl.dat"
		    "${COPYFROMEXEDIR}/../resources/qtwebengine_*.pak" )
	    file( COPY ${WEBENGINE_RESOURCES_FILES}
		  DESTINATION "${COPYTOEXEDIR}/../resources" )
	    unset( WEBENGINE_RESOURCES_FILES )
	endif()

	file( GLOB WEBENGINE_RESOURCES_FILES
		    "${COPYFROMEXEDIR}/../resources/v8_context_snapshot*.bin" )
	if ( NOT "${WEBENGINE_RESOURCES_FILES}" STREQUAL "" )
	    file( COPY ${WEBENGINE_RESOURCES_FILES}
		  DESTINATION "${COPYTOEXEDIR}/../resources"
		  PATTERN "v8_context_snapshot.debug.bin" EXCLUDE )
	endif()

	if ( WIN32 )
	    file( COPY "${COPYFROMEXEDIR}/QtWebEngineProcess.exe"
		  DESTINATION "${COPYTOEXEDIR}" )
	elseif ( APPLE )
	    file( COPY "${COPYFROMEXEDIR}/../Helpers/QtWebEngineProcess.app"
		  DESTINATION "${COPYTOEXEDIR}/../Helpers" )
	else()
	    file( COPY "${COPYFROMEXEDIR}/../libexec/QtWebEngineProcess"
		  DESTINATION "${COPYTOEXEDIR}/../libexec" )
	endif()
    else()

	if ( WIN32 )
	    file( COPY "${COPYFROMEXEDIR}/QtWebEngineProcessd.exe"
		  DESTINATION "${COPYTOEXEDIR}" )
	endif()

	if ( EXISTS "${COPYFROMEXEDIR}/../resources/v8_context_snapshot.debug.bin" )
	    file( COPY "${COPYFROMEXEDIR}/../resources/v8_context_snapshot.debug.bin"
	      DESTINATION "${COPYTOEXEDIR}/../resources" )
	endif()

    endif()

endmacro(COPY_QTWEBENGINE)

macro( COPY_DATAFILES PACKAGE_NAME )

    foreach( SPECFILE ${SPECFILES} )
	 file( COPY "${COPYFROMDIR}/${SPECFILE}"
	       DESTINATION "${COPYTODIR}" )
    endforeach()

    if ( IS_DIRECTORY "${COPYFROMDATADIR}/${PACKAGE_NAME}" )
	file( GLOB SHAREFILES "${COPYFROMDATADIR}/${PACKAGE_NAME}/*" )
	file( COPY "${SHAREFILES}"
	      DESTINATION "${COPYTODATADIR}" )
	unset( SHAREFILES )
    endif()

    foreach( DATADIR ${DATALIST} )
	file( COPY "${COPYFROMDATADIR}/${DATADIR}"
	      DESTINATION "${COPYTODATADIR}" )
    endforeach()

    foreach ( PYTHONREQFNM ${PYTHONREQLIST} )
	file( COPY "${COPYFROMDATADIR}/Python/${PYTHONREQFNM}.txt"
	      DESTINATION "${COPYTODATADIR}/Python" )
    endforeach()

    foreach( LEGALDIR ${LEGALLIST} )
	file( COPY "${COPYFROMDATADIR}/Legal/${LEGALDIR}"
	      DESTINATION "${COPYTODATADIR}/Legal" )
    endforeach()

endmacro(COPY_DATAFILES)

macro( CREATE_PACKAGE PACKAGE_NAME )

    if ( NOT DEFINED COPY_TARGETS )
	set( COPY_TARGETS TRUE )
    endif()

    foreach( LIB IN LISTS LIBLIST PLUGINS SPECMODS )
	if( WIN32 )
	    set( LIBNM "${LIB}${CMAKE_POSTFIX}.dll" )
	elseif( APPLE )
	    set( LIBNM "lib${LIB}${CMAKE_POSTFIX}.dylib" )
	else()
	    set( LIBNM "lib${LIB}${CMAKE_POSTFIX}.so")
	endif()

	if ( EXISTS "${COPYFROMLOCDIR}/${LIBNM}" )
	    if ( COPY_TARGETS )
		COPY_LIB()
	    endif()

	    if ( WIN32 AND "${PACKAGE_NAME}" MATCHES "^devel" )
		file( COPY "${COPYFROMLIBDIR}/${LIB}${CMAKE_POSTFIX}.lib"
		      DESTINATION "${COPYTOLIBDIR}" )
		file( COPY "${COPYFROMLOCDIR}/${LIB}${CMAKE_POSTFIX}.pdb"
		      DESTINATION "${COPYTOLOCDIR}" )
	    endif()
	endif()

    endforeach()

    foreach( EXEC ${EXECLIST} )
	set( EXECFILE "${EXEC}${CMAKE_POSTFIX}" )
	if ( WIN32 )
	    get_filename_component( exec_ext ${EXECFILE} EXT )
	    if ( "${exec_ext}" STREQUAL "" )
		set( EXECFILE "${EXEC}${CMAKE_POSTFIX}.exe" )
		set( PDB_FILE "${EXEC}${CMAKE_POSTFIX}.pdb" )
	    endif()
	    unset( exec_ext )
	endif()

	if ( COPY_TARGETS )
	    file( COPY "${COPYFROMEXEDIR}/${EXECFILE}"
		  DESTINATION "${COPYTOEXEDIR}" )
	endif()
	if ( WIN32 AND "${PACKAGE_NAME}" MATCHES "^devel" AND DEFINED PDB_FILE )
	    file( COPY "${COPYFROMEXEDIR}/${PDB_FILE}"
		  DESTINATION "${COPYTOEXEDIR}" )
	endif()

	unset( EXECFILE )
	unset( PDB_FILE )
    endforeach()

    if ( ${PACKAGE_TYPE} STREQUAL "Production" AND
	 NOT ${PACKAGE_NAME} MATCHES "^devel" )
	foreach( PLUGIN ${PLUGINS} )
	    file( COPY "${COPYFROMDIR}/plugins/${OD_PLFSUBDIR}"
		  DESTINATION "${COPYTODIR}/plugins"
		  FILES_MATCHING
		  PATTERN "*.${PLUGIN}.alo"
		  PATTERN "test_*.*.alo" EXCLUDE )
	endforeach()
    endif()

    COPY_DATAFILES( ${PACKAGE_NAME} )

    foreach( FILES ${ODSCRIPTS} )
	file( GLOB SCRIPTS "${COPYFROMSCRIPTSDIR}/${FILES}" )
	file( COPY ${SCRIPTS}
	      DESTINATION "${COPYTOSCRIPTSDIR}" )
    endforeach()

    foreach( PYDIR ${PYTHONDIR} )
	file( COPY "${COPYFROMSCRIPTSDIR}/python/${PYDIR}"
	      DESTINATION "${COPYTOSCRIPTSDIR}/python" )
    endforeach()

    foreach( FILEORDIR DESTDIR IN ZIP_LISTS OTHERFILES OTHERFILESDEST )
	file( COPY "${FILEORDIR}"
	      DESTINATION "${DESTDIR}" )
    endforeach()

    if ( THIRDPARTY_LIBS )
	COPY_THIRDPARTYLIBS()
    endif()

endmacro(CREATE_PACKAGE)

macro( CREATE_BASEPACKAGE PACKAGE_NAME )
    if( ${PACKAGE_NAME} STREQUAL "basedata" )
	set( ODDGBSTR "od" )
	file( COPY "${COPYFROMDATADIR}/localizations/od_en_US.qm"
	      DESTINATION "${COPYTODATADIR}/localizations" )
	file( COPY "${COPYFROMDIR}/relinfo/README.txt"
	      DESTINATION "${COPYTODIR}/relinfo" )
	file( COPY "${COPYFROMDIR}/relinfo/RELEASEINFO.txt"
	      DESTINATION "${COPYTODIR}/doc/ReleaseInfo" )
    elseif( ${PACKAGE_NAME} STREQUAL "dgbbasedata" )
	set( ODDGBSTR "dgb" )
	file( GLOB QMFILES ${COPYFROMDATADIR}/localizations/*.qm )
	foreach( QMFILE ${QMFILES} )
	    get_filename_component( QMFILENM ${QMFILE} NAME )
	    file( COPY "${COPYFROMDATADIR}/localizations/${QMFILENM}"
		  DESTINATION "${COPYTODATADIR}/localizations"
		  PATTERN "od_en_US*" EXCLUDE )
	 endforeach()
    else()
	message( FATAL_ERROR "Should not be reached" )
    endif()

    file( COPY "${COPYFROMDIR}/doc/Videos.${ODDGBSTR}"
	  DESTINATION "${COPYTODIR}/doc" )

    COPY_DATAFILES( ${PACKAGE_NAME} )
endmacro(CREATE_BASEPACKAGE)

macro( CREATE_DEVELPACKAGE PACKAGE_NAME )

    foreach( LIB ${LIBLIST} )
	file( COPY "${COPYFROMDIR}/include/${LIB}"
	      DESTINATION "${COPYTODIR}/include"
	      PATTERN "*_autogen*" EXCLUDE )
	file( COPY "${COPYFROMDIR}/src/${LIB}"
	      DESTINATION "${COPYTODIR}/src"
	      PATTERN "*_autogen*" EXCLUDE )
    endforeach()

    foreach( PLUGIN ${PLUGINS} )
	file( COPY "${COPYFROMDIR}/plugins/${PLUGIN}"
	      DESTINATION "${COPYTODIR}/plugins"
	      PATTERN "*_autogen*" EXCLUDE )
    endforeach()

    foreach( SPECMOD ${SPECMODS} )
	file( COPY "${COPYFROMDIR}/spec/${SPECMOD}"
	      DESTINATION "${COPYTODIR}/spec"
	      PATTERN "*_autogen*" EXCLUDE )
    endforeach()

    if ( "${PACKAGE_NAME}" STREQUAL "develbatch" )
	file( COPY "${COPYFROMDIR}/CMakeLists.txt"
	      DESTINATION "${COPYTODIR}" )
	file( COPY "${COPYFROMDIR}/CMakeModules"
	      DESTINATION "${COPYTODIR}" )
	file( COPY "${COPYFROMDIR}/doc/Programmer/batchprogexample"
		   "${COPYFROMDIR}/doc/Programmer/pluginexample"
	      DESTINATION "${COPYTODIR}/doc/Programmer" )
	file( COPY "${COPYFROMDIR}/dtect"
	      DESTINATION "${COPYTODIR}" )

	file( GLOB HTMLFILES "${BINARY_DIR}/doc/Programmer/*.html" )
	file( COPY ${HTMLFILES}
	      DESTINATION "${COPYTODIR}/doc/Programmer" )
	file( GLOB PNGFILES "${SOURCE_DIR}/doc/Programmer/*.png" )
	file( COPY ${PNGFILES}
	      DESTINATION "${COPYTODIR}/doc/Programmer" )
    endif()
endmacro(CREATE_DEVELPACKAGE)

macro( CREATE_DOCPACKAGE PACKAGE_NAME )
    if( WIN32 )
	if( ${PACKAGE_NAME} STREQUAL "doc" )
	    file( COPY "${COPYFROMDIR}/doc/od_userdoc"
		  DESTINATION "${COPYTODIR}/doc" )
	elseif( ${PACKAGE_NAME} STREQUAL "dgbdoc" )
	    file( COPY "${COPYFROMDIR}/doc/dgb_userdoc"
		  DESTINATION "${COPYTODIR}/doc" )
	    file( GLOB FILES "${COPYFROMDIR}/doc/flexnet*" )
	    file( COPY ${FILES}
		  DESTINATION "${COPYTODIR}/doc" )
	endif()
    else()
	if( ${PACKAGE_NAME} STREQUAL "classdoc" )
	    if( EXISTS "${COPYFROMDIR}/doc/Programmer/Generated" )
		file( COPY "${COPYFROMDIR}/doc/Programmer/Generated"
		      DESTINATION "${COPYTODIR}/doc/Programmer" )
	    else()
		message( FATAL_ERROR "Class doc not installed correctly. ${PACKAGE_FILENAME} is not self contained." )
	    endif()
	endif()
    endif()
endmacro(CREATE_DOCPACKAGE)

macro( ZIPPACKAGE PACKAGE_FILENAME REL_DIR PACKAGE_DIR )
    execute_process( COMMAND "${ZIP_EXEC}" u -tzip -mmt -mx5 -snl -r -ba -bso0 -bd
			     "${PACKAGE_FILENAME}" "${REL_DIR}"
			     WORKING_DIRECTORY "${PACKAGE_DIR}"
			     RESULT_VARIABLE STATUS
			     OUTPUT_VARIABLE OUTVAR
			     ERROR_VARIABLE ERRVAR )

    if( NOT ${STATUS} EQUAL 0 )
	message( FATAL_ERROR "Failed to create zip file ${PACKAGE_FILENAME}: ${STATUS}" )
    endif()

    file( REMOVE_RECURSE "${PACKAGE_DIR}/${REL_DIR}" )
endmacro(ZIPPACKAGE)
