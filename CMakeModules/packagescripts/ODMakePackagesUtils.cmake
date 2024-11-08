#________________________________________________________________________
#
# Copyright:	dGB Beheer B.V.
# License:	https://dgbes.com/index.php/licensing
#________________________________________________________________________
#
# CMake script to build a release
#

macro ( CREATE_PACKAGE PACKAGE_NAME )
    if( ${PACKAGE_NAME} STREQUAL "base" )
	if( UNIX )
	    if ( APPLE )
		file( COPY ${COPYFROMDATADIR}/qt_menu.nib
		      DESTINATION ${COPYTODATADIR} )
		file( COPY ${COPYFROMDATADIR}/od.icns
		      DESTINATION ${COPYTODATADIR} )
	    else()
		file(COPY "${COPYFROMLIBDIR}/../lib"
		     DESTINATION "${COPYTOLIBDIR}/../" )
	    endif()
	endif()
	if ( WIN32 )
	    file( GLOB ML_FW_RULENM "${COPYFROMDATADIR}/data/Firewall/od_main.txt"
				"${COPYFROMDATADIR}/data/Firewall/od_remoteservice.txt"
				"${COPYFROMDATADIR}/data/Firewall/od_SeisMMBatch.txt" )
	    file( COPY ${ML_FW_RULENM}
			DESTINATION "${COPYTODATADIR}/data/Firewall" )
	endif()

	file( GLOB SURV_PROVS "${COPYFROMDATADIR}/data/SurveyProviders/uiSEGYTools.txt"
		"${COPYFROMDATADIR}/data/SurveyProviders/uiWell.txt" )
	file( COPY ${SURV_PROVS}
	      DESTINATION "${COPYTODATADIR}/data/SurveyProviders" )

	file(COPY ${COPYFROMDATADIR}/bin/${OD_PLFSUBDIR}/lm.dgb
	     DESTINATION ${COPYTODATADIR}/bin/${OD_PLFSUBDIR}
	     FILES_MATCHING PATTERN lmutil* )

	if( NOT "${MATLAB_DIR}" STREQUAL "" )
	    if ( NOT EXISTS "${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/MATLAB" )
		message( FATAL_ERROR "MATLAB directory not found" )
	    endif()
	    file( COPY ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/MATLAB
		  DESTINATION ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR} )
	endif()

	COPY_THIRDPARTYLIBS()
	list( APPEND LIBLIST ${PLUGINS} )
    elseif( ${PACKAGE_NAME} STREQUAL "dgbpro" )
	file( GLOB FILESEL_PROVS "${COPYFROMDATADIR}/data/FileSelProviders/uidGB*.txt" )
	file( COPY ${FILESEL_PROVS}
	      DESTINATION "${COPYTODATADIR}/data/FileSelProviders" )
	file( GLOB OPENSSL_PROVS "${COPYFROMDATADIR}/data/OpenSSL/*.txt" )
	file( COPY ${OPENSSL_PROVS}
	      DESTINATION "${COPYTODATADIR}/data/OpenSSL" )
	file( GLOB SEISTRCTR_PROVS "${COPYFROMDATADIR}/data/SeisTrcTranslators/dGB*.txt" )
	file( COPY ${SEISTRCTR_PROVS}
	      DESTINATION "${COPYTODATADIR}/data/SeisTrcTranslators" )
	file( GLOB SURV_PROVS "${COPYFROMDATADIR}/data/SurveyProviders/uidGB*.txt" )
	file( COPY ${SURV_PROVS}
	      DESTINATION "${COPYTODATADIR}/data/SurveyProviders" )

	COPY_THIRDPARTYLIBS()
    elseif( ${PACKAGE_NAME} STREQUAL "dgbbatch" )
	file( GLOB OPENSSL_PROVS "${COPYFROMDATADIR}/data/OpenSSL/*.txt" )
	file( COPY ${OPENSSL_PROVS}
	      DESTINATION "${COPYTODATADIR}/data/OpenSSL" )
	file( GLOB SEISTRCTR_PROVS "${COPYFROMDATADIR}/data/SeisTrcTranslators/dGB*.txt" )
	file( COPY ${SEISTRCTR_PROVS}
	      DESTINATION "${COPYTODATADIR}/data/SeisTrcTranslators" )
	foreach( LIB ${EXTERNAL_BACKEND_LIBS} )
	    file( GLOB EXTERNALLIB "${COPYFROMLIBDIR}/lib${LIB}.so*" )
	    file( COPY ${EXTERNALLIB} DESTINATION ${COPYTOLIBDIR} )
	endforeach()
	foreach( LIBGROUP ${EXTERNAL_BACKEND_LIBGROUPS} )
	    file( GLOB EXTERNALLIB "${COPYFROMLIBDIR}/lib${LIBGROUP}*" )
	    file( COPY ${EXTERNALLIB} DESTINATION ${COPYTOLIBDIR} )
	endforeach()
    elseif( WIN32 AND ${PACKAGE_NAME} STREQUAL "dgbml" )
	file( GLOB ML_FW_RULENM "${COPYFROMDATADIR}/data/Firewall/od_DeepLearning*.txt" )
	file( COPY ${ML_FW_RULENM}
			DESTINATION "${COPYTODATADIR}/data/Firewall" )
    endif()

    #TODO Need to check whether we need to use this macro on MAC.
    if ( APPLE AND SYSTEMLIBS ) #TODO Need to check whether we	need to use this macro on MAC.
	COPY_MAC_SYSTEMLIBS()
	unset( SYSTEMLIBS )
    endif()

    foreach( LIB ${LIBLIST} )
	if( WIN32 )
	    set( LIBNM "${LIB}.dll" )
	    #Stripping not required on windows
	elseif( APPLE )
	    set( LIBNM "lib${LIB}.dylib" )
	    execute_process( COMMAND ${OpendTect_DIR}/data/install_files/macscripts/chfwscript ${COPYFROMLIBDIR}/${LIBNM} )
	    #Not using breakpad on MAC
	else()
	    set( LIBNM "lib${LIB}.so")
	endif()
	if ( ("${CMAKE_BUILD_TYPE}" STREQUAL "Release" AND
	      NOT "${PACKAGE_NAME}" STREQUAL "devel") OR
	     ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" AND
	      "${PACKAGE_NAME}" STREQUAL "devel") )
	    file( COPY ${COPYFROMLIBDIR}/${LIBNM}
		  DESTINATION ${COPYTOLIBDIR} )
	endif()

	if ( "${CMAKE_BUILD_TYPE}" STREQUAL "Release" AND
	     NOT "${PACKAGE_NAME}" STREQUAL "devel" )
	    file( GLOB ALOFILES ${COPYFROMDATADIR}/plugins/${OD_PLFSUBDIR}/*.${LIB}.alo )
	    foreach( ALOFILE ${ALOFILES} )
		file( COPY ${ALOFILE}
		      DESTINATION ${COPYTODATADIR}/plugins/${OD_PLFSUBDIR} )
	    endforeach()
	endif()
    endforeach()

    foreach( PYDIR ${PYTHONDIR} )
	file( COPY ${COPYFROMDATADIR}/bin/python/${PYDIR}
	      DESTINATION ${COPYTODATADIR}/bin/python )
    endforeach()
    unset( PYTHONDIR )

    if( ${PACKAGE_NAME} STREQUAL "dgbbase" )
#Install lm
	foreach( SPECFILE ${SPECFILES} )
	     file( COPY ${COPYFROMDATADIR}/${SPECFILE}
		   DESTINATION ${COPYTODATADIR} )
	endforeach()

	file( COPY ${COPYFROMDATADIR}/bin/${OD_PLFSUBDIR}/lm.dgb
	      DESTINATION ${COPYTODATADIR}/bin/${OD_PLFSUBDIR}
	      FILES_MATCHING PATTERN dgbld*
			     PATTERN lmgrd*
			     PATTERN lmtools* )
	if ( WIN32 )
	    file( COPY ${COPYFROMDATADIR}/bin/${OD_PLFSUBDIR}/odExternal
		  DESTINATION ${COPYTODATADIR}/bin/${OD_PLFSUBDIR}
		  PATTERN "*dd.exe" EXCLUDE
		  PATTERN "*rd.exe" EXCLUDE )
	elseif( NOT APPLE )
	    file( COPY ${COPYFROMDATADIR}/bin/${OD_PLFSUBDIR}/libexec
		  DESTINATION ${COPYTODATADIR}/bin/${OD_PLFSUBDIR} )
	endif()
    endif()

    if ( WIN32 AND ${PACKAGE_NAME} STREQUAL "base" )
	list( APPEND EXECLIST ${WINEXECLIST} )
    endif()

    if ( ("${CMAKE_BUILD_TYPE}" STREQUAL "Release" AND
	  NOT "${PACKAGE_NAME}" STREQUAL "devel") OR
	 ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug" AND
	  "${PACKAGE_NAME}" STREQUAL "devel") )
	foreach( EXE ${EXECLIST} )
	    if( WIN32 )
		file( COPY ${COPYFROMLIBDIR}/${EXE}.exe
		      DESTINATION ${COPYTOLIBDIR} )
	    elseif( APPLE )
		execute_process( COMMAND ${OpendTect_DIR}/data/install_files/macscripts/chfwscript ${COPYFROMEXEDIR}/${EXE} )
		file( COPY ${COPYFROMEXEDIR}/${EXE}
		      DESTINATION ${COPYTOEXEDIR} )
	    else()
		file( COPY ${COPYFROMLIBDIR}/${EXE}
		      DESTINATION ${COPYTOLIBDIR} )
	    endif()
	endforeach()
    endif()

    if( ${PACKAGE_NAME} STREQUAL "base" )
	foreach( SPECFILE ${SPECFILES} )
	     file( COPY ${COPYFROMDATADIR}/${SPECFILE}
		   DESTINATION ${COPYTODATADIR} )
	endforeach()
	foreach( FILES ${ODSCRIPTS} )
	     file( GLOB SCRIPTS ${COPYFROMDATADIR}/bin/${FILES} )
	     foreach( SCRIPT ${SCRIPTS} )
		file( COPY ${SCRIPT}
		      DESTINATION ${COPYTODATADIR}/bin )
	     endforeach()
	endforeach()

	if( WIN32 )
	    file( COPY ${COPYFROMLIBDIR}/od_main_debug.bat
		  DESTINATION ${COPYTOLIBDIR} )
	endif()
    endif()

    if( ${PACKAGE_NAME} STREQUAL "odbatch" )
	COPY_BUNDLED_LIBS( systemlibs "${SYSTEMLIBS}" )
	COPY_BUNDLED_LIBS( OpenSSL "${OPENSSLLIBS}" )
	file( COPY ${COPYFROMLIBDIR}/qt.conf
	      DESTINATION ${COPYTOLIBDIR} )
	foreach( LIB ${EXTERNAL_BACKEND_LIBS} )
	    file( GLOB EXTERNALLIB "${COPYFROMLIBDIR}/lib${LIB}.so*" )
	    file( COPY ${EXTERNALLIB} DESTINATION ${COPYTOLIBDIR} )
	endforeach()
    endif()

    foreach( EXTERNALLIB ${EXTERNALLIBS} )
	file( COPY ${COPYFROMLIBDIR}/${EXTERNALLIB}
	      DESTINATION ${COPYTOLIBDIR} )
    endforeach()
    set( EXTERNALLIBS "")

    foreach( DATADIR ${DATADIRLIST} )
	file( COPY ${COPYFROMDATADIR}/data/${DATADIR}
	      DESTINATION ${COPYTODATADIR}/data )
    endforeach()
    foreach( LEGALDIR ${LEGALLIST} )
	file( COPY ${COPYFROMDATADIR}/data/Legal/${LEGALDIR}
	      DESTINATION ${COPYTODATADIR}/data/Legal )
    endforeach()
    if ( DEFINED PYTHONREQLIST )
	foreach ( PYTHONREQFNM ${PYTHONREQLIST} )
	    file( COPY "${COPYFROMDATADIR}/data/Python/${PYTHONREQFNM}.txt"
		  DESTINATION "${COPYTODATADIR}/data/Python" )
	endforeach()
    endif()
    unset( PYTHONREQLIST )
    unset( LEGALLIST )
    unset( DATADIRLIST )

    ZIPPACKAGE( ${PACKAGE_FILENAME} ${REL_DIR} ${PACKAGE_DIR} )
endmacro( CREATE_PACKAGE )

macro( COPY_BUNDLED_LIBS BUNDLENAME LIBS )
    message( "LIBS: ${LIBS}" )
    message( "BUNDLENAME: ${BUNDLENAME}" )
    if( NOT EXISTS ${COPYTOLIBDIR}/${BUNDLENAME} )
	file( MAKE_DIRECTORY ${COPYTOLIBDIR}/${BUNDLENAME} )
    endif()
    foreach( LIB ${LIBS} )
	file( GLOB LIBFILE "${COPYFROMLIBDIR}/lib${LIB}.so*" )
	file( COPY ${LIBFILE} DESTINATION ${COPYTOLIBDIR}/${BUNDLENAME} )
    endforeach()
endmacro( COPY_BUNDLED_LIBS )

macro( COPY_THIRDPARTYLIBS )
    list( APPEND SYSLIBS ${SYSTEMLIBS} )
    list( APPEND SSLLIBS ${OPENSSLLIBS} )
    set( FONTLIBS fontconfig;freetype;png15;png16 )
    foreach( LIB ${OD_THIRD_PARTY_FILES} )
	string( FIND ${LIB} "Qt" ISQTLIB )
	if ( APPLE AND NOT ${ISQTLIB} EQUAL -1 )
	    file( MAKE_DIRECTORY ${COPYTOLIBDIR}/${LIB}.framework
				 ${COPYTOLIBDIR}/${LIB}.framework/Versions
				 ${COPYTOLIBDIR}/${LIB}.framework/Versions/${QT_VERSION_MAJOR} )
	    file( COPY ${COPYFROMLIBDIR}/${LIB}
		  DESTINATION ${COPYTOLIBDIR}/${LIB}.framework/Versions/${QT_VERSION_MAJOR} )
	else()
	    if ( WIN32 )
		get_filename_component( PDBFILE ${LIB} LAST_EXT )
		if( ${PDBFILE} STREQUAL ".pdb" )
		    continue()
		endif()
	    else()
		if ( APPLE )
		    execute_process( COMMAND ${OpendTect_DIR}/data/install_files/macscripts/chfwscript ${COPYFROMLIBDIR}/${LIB} )
		else()
		    list( FIND SYSLIBS "${LIB}" ITEMIDX )
		    if ( NOT ${ITEMIDX} EQUAL -1 )
			if( NOT EXISTS ${COPYTOLIBDIR}/systemlibs )
			    file( MAKE_DIRECTORY ${COPYTOLIBDIR}/systemlibs )
			endif()

			file( COPY ${COPYFROMLIBDIR}/${LIB}
			      DESTINATION ${COPYTOLIBDIR}/systemlibs )
			continue()
		    endif()

		    foreach( FONTLIB ${FONTLIBS} )
			string( FIND ${LIB} ${FONTLIB} ISFONTLIB )
			if ( NOT ISFONTLIB EQUAL -1 )
			    file( COPY ${COPYFROMLIBDIR}/${FONTLIB}
				  DESTINATION ${COPYTOLIBDIR} )
			    break()
			endif()
		    endforeach()
		    if ( NOT ISFONTLIB EQUAL -1 )
			continue()
		    endif()
		endif()

		list( FIND SSLLIBS "${LIB}" ITEMIDX )
		if ( NOT ${ITEMIDX} EQUAL -1 )
		    if ( APPLE )
			if( NOT EXISTS ${COPYTODATADIR}/OpenSSL )
			    file( MAKE_DIRECTORY ${COPYTODATADIR}/OpenSSL )
			endif()
			file( COPY ${COPYFROMLIBDIR}/${LIB}
			      DESTINATION ${COPYTODATADIR}/OpenSSL )
		    else()
			if( NOT EXISTS ${COPYTOLIBDIR}/OpenSSL )
			    file( MAKE_DIRECTORY ${COPYTOLIBDIR}/OpenSSL )
			endif()
			file( COPY ${COPYFROMLIBDIR}/${LIB}
			      DESTINATION ${COPYTOLIBDIR}/OpenSSL )
		    endif()
		    continue()
		endif()
	    endif()

	    file( COPY ${COPYFROMLIBDIR}/${LIB}
		  DESTINATION ${COPYTOLIBDIR} )
	endif()

    endforeach()

    foreach( ODPLUGIN ${OD_QTPLUGINS} )
	if ( "${ODPLUGIN}" STREQUAL "resources" )
	    if ( APPLE )
		file( COPY ${COPYFROMLIBDIR}/../PlugIns/resources
		      DESTINATION ${COPYTOLIBDIR}/../PlugIns )
	    else()
		file( COPY ${COPYFROMLIBDIR}/../resources
		      DESTINATION ${COPYTODATADIR}/bin/${OD_PLFSUBDIR} )
	    endif()
	else()
	    if ( APPLE )
		file( COPY ${COPYFROMLIBDIR}/../PlugIns/${ODPLUGIN}
		      DESTINATION ${COPYTOLIBDIR}/../PlugIns )
	    else()
		file( COPY ${COPYFROMLIBDIR}/../plugins/${ODPLUGIN}
		      DESTINATION ${COPYTODATADIR}/bin/${OD_PLFSUBDIR}/plugins )
	    endif()
	endif()
    endforeach()

    foreach( TRANSLATION_FILE ${OD_QT_TRANSLATION_FILES} )
	get_filename_component( QTWEB_LOCALS_FILE ${TRANSLATION_FILE} EXT )
	if( ${QTWEB_LOCALS_FILE} STREQUAL ".pak" )
	    if ( APPLE )
		file( COPY ${COPYFROMLIBDIR}/../PlugIns/translations/qtwebengine_locales/${TRANSLATION_FILE}
		      DESTINATION ${COPYTOLIBDIR}/../PlugIns/translations/qtwebengine_locales )
	    else()
		file( COPY ${COPYFROMLIBDIR}/../translations/qtwebengine_locales/${TRANSLATION_FILE}
		      DESTINATION ${COPYTODATADIR}/bin/${OD_PLFSUBDIR}/translations/qtwebengine_locales )
	    endif()
	else()
	    if ( APPLE )
		file( COPY ${COPYFROMLIBDIR}/../PlugIns/translations/${TRANSLATION_FILE}
		      DESTINATION ${COPYTOLIBDIR}/../PlugIns/translations )
	    else()
		file( COPY ${COPYFROMLIBDIR}/../translations/${TRANSLATION_FILE}
		      DESTINATION ${COPYTODATADIR}/bin/${OD_PLFSUBDIR}/translations )
	    endif()
	endif()
    endforeach()

endmacro( COPY_THIRDPARTYLIBS )

macro( COPY_MAC_SYSTEMLIBS )
    if( APPLE )
	foreach( SYSLIB ${SYSTEMLIBS} )
	    execute_process( COMMAND ${OpendTect_DIR}/data/install_files/macscripts/chfwscript ${COPYFROMLIBDIR}/${SYSLIB} )
	    file( COPY ${COPYFROMLIBDIR}/${SYSLIB}
		  DESTINATION ${COPYTOLIBDIR} )
	endforeach()
    endif()
endmacro( COPY_MAC_SYSTEMLIBS )


macro( CREATE_BASEPACKAGES PACKAGE_NAME )
    string( FIND ${PACKAGE_NAME} "dgb" STATUS )
    if( ${STATUS} EQUAL "0" )
	set( ODDGBSTR "dgb" )

	#add the base translation to basedata, not dgbbasedata
	if( APPLE )
	    file( RENAME "${PACKAGE_DIR}/${REL_DIR}" "${PACKAGE_DIR}/${REL_DIR}_TMP" )
	    file( COPY "${COPYFROMDATADIR}/data/localizations/od_en_US.qm"
		  DESTINATION "${COPYTODATADIR}/data/localizations" )
	    ZIPPACKAGE( "basedata_mac.zip" ${REL_DIR} ${PACKAGE_DIR} )
	    file( RENAME "${PACKAGE_DIR}/${REL_DIR}_TMP" "${PACKAGE_DIR}/${REL_DIR}" )
	else()
	    file( RENAME "${COPYTODATADIR}" "${COPYTODATADIR}_TMP" )
	    file( COPY "${COPYFROMDATADIR}/data/localizations/od_en_US.qm"
		  DESTINATION "${COPYTODATADIR}/data/localizations" )
	    ZIPPACKAGE( "basedata.zip" ${REL_DIR} ${PACKAGE_DIR} )
	    file( RENAME "${COPYTODATADIR}_TMP" "${COPYTODATADIR}" )
	endif()

	file( GLOB QMFILES ${COPYFROMDATADIR}/data/localizations/*.qm )
	foreach( QMFILE ${QMFILES} )
	    get_filename_component( QMFILENM ${QMFILE} NAME )
	    file( COPY "${COPYFROMDATADIR}/data/localizations/${QMFILENM}"
		  DESTINATION "${COPYTODATADIR}/data/localizations"
		  PATTERN "od_en_US*" EXCLUDE )
	 endforeach()
    else()
	set( ODDGBSTR "od" )
	file( COPY ${COPYFROMDATADIR}/relinfo/README.txt
	      DESTINATION ${COPYTODATADIR}/relinfo )
	file( COPY ${COPYFROMDATADIR}/relinfo/RELEASEINFO.txt
	      DESTINATION ${COPYTODATADIR}/doc/ReleaseInfo )
    endif()

    file( COPY ${COPYFROMDATADIR}/doc/Videos.${ODDGBSTR}
	  DESTINATION ${COPYTODATADIR}/doc )

    foreach( LIBS ${LIBLIST} )
	file( GLOB DATAFILES ${COPYFROMDATADIR}/data/${LIBS} )
	foreach( DATA ${DATAFILES} )
	    get_filename_component( DATALIBNM ${DATA} NAME )
	    file( COPY ${COPYFROMDATADIR}/data/${DATALIBNM}
		  DESTINATION ${COPYTODATADIR}/data )
	 endforeach()
    endforeach()

    foreach( DATADIR ${DATADIRLIST} )
	file( COPY ${COPYFROMDATADIR}/data/${DATADIR}
	      DESTINATION ${COPYTODATADIR}/data )
    endforeach()
    foreach( LEGALDIR ${LEGALLIST} )
	file( COPY ${COPYFROMDATADIR}/data/Legal/${LEGALDIR}
	      DESTINATION ${COPYTODATADIR}/data/Legal )
    endforeach()
    if ( DEFINED PYTHONREQLIST )
	foreach ( PYTHONREQFNM ${PYTHONREQLIST} )
	    file( COPY "${COPYFROMDATADIR}/data/Python/${PYTHONREQFNM}.txt"
		  DESTINATION "${COPYTODATADIR}/data/Python" )
	endforeach()
    endif()
    unset( PYTHONREQLIST )
    unset( LEGALLIST )
    unset( DATADIRLIST )

    ZIPPACKAGE( ${PACKAGE_FILENAME} ${REL_DIR} ${PACKAGE_DIR} )
endmacro( CREATE_BASEPACKAGES )


macro( INIT_DESTINATIONDIR PACKAGE_NAME )
    set( PACKAGE_FILENAME ${PACKAGE_NAME} )
    set( PACKAGE_FILENAME "${PACKAGE_FILENAME}_${OD_PLFSUBDIR}.zip" )
    set( VER_FILENAME "${PACKAGE_NAME}_${OD_PLFSUBDIR}" )
    if( ${PACKAGE_NAME} STREQUAL "basedata" )
	set( VER_FILENAME "basedata" )
	set( PACKAGE_FILENAME "basedata.zip" )
	if( APPLE )
	    set( PACKAGE_FILENAME "basedata_mac.zip" )
	    set( VER_FILENAME "basedata_mac" )
	endif()
    elseif( ${PACKAGE_NAME} STREQUAL "dgbbasedata" )
	set( VER_FILENAME "dgbbasedata" )
	set( PACKAGE_FILENAME "dgbbasedata.zip" )
	if( APPLE )
	    set( PACKAGE_FILENAME "dgbbasedata_mac.zip" )
	    set( VER_FILENAME "dgbbasedata_mac" )
	endif()
    elseif( ${PACKAGE_NAME} STREQUAL "doc" )
	set( VER_FILENAME "doc" )
	set( PACKAGE_FILENAME "doc.zip" )
	if( APPLE )
	    set( PACKAGE_FILENAME "doc_mac.zip" )
	    set( VER_FILENAME "doc_mac" )
	endif()
    elseif( ${PACKAGE_NAME} STREQUAL "dgbdoc" )
	set( VER_FILENAME "dgbdoc" )
	set( PACKAGE_FILENAME "dgbdoc.zip" )
	if( APPLE )
	    set( PACKAGE_FILENAME "dgbdoc_mac.zip" )
	    set( VER_FILENAME "dgbdoc_mac" )
	endif()
    elseif( ${PACKAGE_NAME} STREQUAL "classdoc" )
	set( VER_FILENAME "classdoc" )
	set( PACKAGE_FILENAME "classdoc.zip" )
	if( APPLE )
	    set( PACKAGE_FILENAME "classdoc_mac.zip" )
	    set( VER_FILENAME "classdoc_mac" )
	endif()
    endif()

    if( NOT EXISTS ${PACKAGE_DIR} )
	file( MAKE_DIRECTORY ${PACKAGE_DIR} )
    endif()

    #Store OpendTect_INST_DIR in another variable to handle installation directory on MAC platform
    set( REL_DIR "${OpendTect_INST_DIR}" )
    set( FULLVER_NAME "${OpendTect_FULL_VERSION}" )
    if( APPLE )
	set( REL_DIR "OpendTect\ ${REL_DIR}.app/Contents" )
    endif()

    set( DESTINATION_DIR "${PACKAGE_DIR}/${REL_DIR}" )
    if( EXISTS "${DESTINATION_DIR}" )
	file( REMOVE_RECURSE "${DESTINATION_DIR}" )
    endif()

    if ( NOT APPLE )
	if( NOT ${PACKAGE_NAME} STREQUAL "basedata" AND
	    NOT ${PACKAGE_NAME} STREQUAL "dgbbasedata" )
	    file( MAKE_DIRECTORY ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/${CMAKE_BUILD_TYPE} )
	    set( COPYFROMLIBDIR ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/${CMAKE_BUILD_TYPE} )
	    set( COPYTOLIBDIR ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/${CMAKE_BUILD_TYPE} )
	endif()

	set( COPYFROMDATADIR ${CMAKE_INSTALL_PREFIX} )
	set( COPYTODATADIR ${DESTINATION_DIR} )
    else()
	file( MAKE_DIRECTORY ${DESTINATION_DIR}
			     ${DESTINATION_DIR}/MacOS ${DESTINATION_DIR}/Frameworks )
	set( COPYFROMLIBDIR ${CMAKE_INSTALL_PREFIX}/Contents/Frameworks )
	set( COPYTOLIBDIR ${DESTINATION_DIR}/Frameworks )
	set( COPYFROMEXEDIR ${CMAKE_INSTALL_PREFIX}/Contents/MacOS )
	set( COPYTOEXEDIR ${DESTINATION_DIR}/MacOS )
	set( COPYFROMDATADIR ${CMAKE_INSTALL_PREFIX}/Contents/Resources )
	set( COPYTODATADIR ${DESTINATION_DIR}/Resources )
    endif()

    if( NOT ( "${CMAKE_BUILD_TYPE}" STREQUAL "Release" AND
	      "${PACKAGE_NAME}" STREQUAL "devel") )
	if ( EXISTS ${PACKAGE_DIR}/${PACKAGE_FILENAME} )
	    file( REMOVE_RECURSE ${PACKAGE_DIR}/${PACKAGE_FILENAME} )
	endif()

	file( WRITE ${COPYTODATADIR}/relinfo/ver.${VER_FILENAME}.txt ${FULLVER_NAME} )
	file( APPEND ${COPYTODATADIR}/relinfo/ver.${VER_FILENAME}.txt "\n" )
	if( APPLE )
	    file( COPY ${CMAKE_INSTALL_PREFIX}/Contents/Info.plist
		  DESTINATION ${DESTINATION_DIR} )
	endif()
    endif()


    if ( NOT "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" OR
	 "${PACKAGE_NAME}" STREQUAL "devel" )
	message( STATUS "Preparing package ${VER_FILENAME}.zip ......" )
    endif()
endmacro( INIT_DESTINATIONDIR )


macro( CREATE_DEVELPACKAGES )
    if ( "${CMAKE_BUILD_TYPE}" STREQUAL "Debug" )
	COPY_THIRDPARTYLIBS()
	file( MAKE_DIRECTORY ${COPYFROMDATADIR}/doc
			     ${COPYTODATADIR}/doc/Programmer)
	file( COPY ${SOURCE_DIR}/CMakeLists.txt
	      DESTINATION ${COPYTODATADIR} )
	file( COPY ${COPYFROMDATADIR}/doc/Programmer/batchprogexample
		   ${COPYFROMDATADIR}/doc/Programmer/pluginexample
	      DESTINATION ${COPYTODATADIR}/doc/Programmer )
	file( COPY ${COPYFROMDATADIR}/dtect
	      DESTINATION ${COPYTODATADIR} )

	foreach( SPECFILE ${SPECFILES} )
	    file( COPY ${COPYFROMDATADIR}/doc/Programmer/${SPECFILE}
		  DESTINATION ${COPYTODATADIR}/doc )
	endforeach()

	file( GLOB HTMLFILES ${BINARY_DIR}/doc/Programmer/*.html )
	foreach( HTMLFILE ${HTMLFILES} )
	    file( COPY ${HTMLFILE}
		  DESTINATION ${COPYTODATADIR}/doc/Programmer )
	endforeach()
	file( GLOB PNGFILES ${SOURCE_DIR}/doc/Programmer/*.png )
	foreach( PNGFILE ${PNGFILES} )
	    file( COPY ${PNGFILE}
		  DESTINATION ${COPYTODATADIR}/doc/Programmer )
	endforeach()

	foreach( DIR CMakeModules include src plugins spec )
	    if( "${DIR}" STREQUAL "plugins" )
		foreach( ODPLUGIN ${ODPLUGINS} )
		    file( COPY ${COPYFROMDATADIR}/plugins/${ODPLUGIN}
			  DESTINATION ${COPYTODATADIR}/plugins )
		endforeach()
	    else()
		file( COPY ${COPYFROMDATADIR}/${DIR}
		      DESTINATION ${COPYTODATADIR} )
	    endif()
	endforeach()

	if ( WIN32 )
	    file( COPY ${SOURCE_DIR}/bin/od_cr_dev_env.bat
		  DESTINATION ${DESTINATION_DIR}/bin )
	else()
	    file( COPY ${COPYFROMDATADIR}/bin/od_cr_dev_env.csh
		  DESTINATION ${COPYTODATADIR} )
	endif()
    endif()

    if( WIN32 )
	file( MAKE_DIRECTORY ${DESTINATION_DIR}/bin
			     ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}
			     ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/${CMAKE_BUILD_TYPE} )

	set( DEVELLIBS ${ODLIBLIST} ${ODPLUGINS} ${SPECSOURCES} )
	#Copying dll, pdb and lib files.
	if ( "${CMAKE_BUILD_TYPE}" STREQUAL "Release" )
	    foreach( DLIB ${DEVELLIBS} )
		if ( EXISTS "${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/${CMAKE_BUILD_TYPE}/${DLIB}.lib" )
		    #Some modules have no library
		    file( COPY ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/${CMAKE_BUILD_TYPE}/${DLIB}.lib
			  DESTINATION ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/${CMAKE_BUILD_TYPE} )
		endif()
	    endforeach()
	else()
	    foreach( DLIB ${DEVELLIBS} )
		file( GLOB FILES ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/${CMAKE_BUILD_TYPE}/${DLIB}.* )
		string( TOLOWER ${DLIB} PDBLIB )
		file( GLOB PDBFILES ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/${CMAKE_BUILD_TYPE}/${PDBLIB}.* )
		set( FILES ${FILES} ${PDBFILES} )
		foreach( FIL ${FILES} )
		    if ( EXISTS "${FIL}" ) #Some modules have no library
			file( COPY ${FIL}
			      DESTINATION ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/${CMAKE_BUILD_TYPE} )
		    endif()
		endforeach()
	    endforeach()

	    #Copying executables and pdb files
	    foreach( EXELIB ${EXECLIST} )
		file( GLOB EXEFILES ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/${CMAKE_BUILD_TYPE}/${EXELIB}.* )
		string( TOLOWER ${EXELIB} EXEPDBLIB )
		file( GLOB EXEPDBFILES ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/${CMAKE_BUILD_TYPE}/${EXEPDBLIB}.* )
		set( EXEILES ${EXEFILES} ${EXEPDBFILES} )
		foreach( ELIB ${EXEILES} )
		    file( COPY ${ELIB}
			  DESTINATION ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/${CMAKE_BUILD_TYPE} )
		endforeach()
	    endforeach()
	endif()
    endif()

    ZIPPACKAGE( ${PACKAGE_FILENAME} ${REL_DIR} ${PACKAGE_DIR} )
endmacro( CREATE_DEVELPACKAGES )

macro( CREATE_DOCPACKAGES PACKAGE_NAME )
    if( WIN32 )
	if( ${PACKAGE_NAME} STREQUAL "doc" )
	    file( COPY ${CMAKE_INSTALL_PREFIX}/doc/od_userdoc
		  DESTINATION ${DESTINATION_DIR}/doc )
	elseif( ${PACKAGE_NAME} STREQUAL "dgbdoc" )
	    file( COPY ${CMAKE_INSTALL_PREFIX}/doc/dgb_userdoc
		  DESTINATION ${DESTINATION_DIR}/doc )
	    file( GLOB FILES ${CMAKE_INSTALL_PREFIX}/doc/flexnet* )
	    foreach( FIL ${FILES} )
		file( COPY ${FIL}
		      DESTINATION ${DESTINATION_DIR}/doc )
	    endforeach()
	endif()
    else()
	if( ${PACKAGE_NAME} STREQUAL "classdoc" )
	    if( EXISTS ${CMAKE_INSTALL_PREFIX}/doc/Programmer/Generated )
		file( COPY ${CMAKE_INSTALL_PREFIX}/doc/Programmer/Generated
		      DESTINATION ${DESTINATION_DIR}/doc/Programmer )
	    else()
		message( FATAL_ERROR "Class doc not installed correctly. ${PACKAGE_FILENAME} is not self contained." )
	    endif()
	endif()
    endif()
    ZIPPACKAGE( ${PACKAGE_FILENAME} ${REL_DIR} ${PACKAGE_DIR} )
endmacro( CREATE_DOCPACKAGES )

macro( ZIPPACKAGE PACKAGE_FILENAME REL_DIR PACKAGE_DIR )
    if( WIN32 )
	execute_process( COMMAND ${OpendTect_DIR}/bin/win64/zip -r -q -u
				 "${PACKAGE_FILENAME}" ${REL_DIR}
				 WORKING_DIRECTORY ${PACKAGE_DIR}
				 RESULT_VARIABLE STATUS
				 OUTPUT_VARIABLE OUTVAR
				 ERROR_VARIABLE ERRVAR )
    else()
	execute_process( COMMAND zip -r -q -u -y
				 "${PACKAGE_FILENAME}" ${REL_DIR}
				 WORKING_DIRECTORY ${PACKAGE_DIR}
				 RESULT_VARIABLE STATUS
				 OUTPUT_VARIABLE OUTVAR
				 ERROR_VARIABLE ERRVAR )
    endif()

    #Error code 12 == "Nothing to do": can be ignored
    if( NOT ${STATUS} EQUAL "0" AND NOT ${STATUS} EQUAL "12" )
	message( FATAL_ERROR "Failed to create zip file ${PACKAGE_FILENAME}: ${STATUS}" )
    endif()

    file( REMOVE_RECURSE "${PACKAGE_DIR}/${REL_DIR}" )
endmacro( ZIPPACKAGE )

