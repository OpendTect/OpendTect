#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:	CMake script to build a release
# Author:	Nageswara
# Date:		August 2012		

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
	set( LIBLIST ${LIBLIST};${PLUGINS} )
    elseif( ${PACKAGE_NAME} STREQUAL "dgbpro" )
	COPY_THIRDPARTYLIBS()
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

	    #copying breakpad symbols
	    if ( OD_ENABLE_BREAKPAD )
		if( WIN32 )
		    #TODO
		elseif( APPLE )
		    #TODO
		else()
		    file( COPY ${COPYFROMLIBDIR}/symbols/${LIBNM}
			  DESTINATION ${COPYTOLIBDIR}/symbols )
		endif()
	    endif()
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
    foreach( PYFILE ${PYTHONFILES} )
	file( COPY ${COPYFROMDATADIR}/bin/python/${PYFILE}
	      DESTINATION ${COPYTODATADIR}/bin/python )
    endforeach()
    unset( PYTHONDIR )
    unset( PYTHONFILES )

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

    if( WIN32 )
	if( ${PACKAGE_NAME} STREQUAL "base" )
	    set( EXECLIST "${EXECLIST};${WINEXECLIST}" )
	endif()
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

	    #copying breakpad symbols
	    if ( OD_ENABLE_BREAKPAD )
		if ( WIN32 )
		    #TODO
		elseif( APPLE )
		    #TODO
		else()
		    file( COPY ${COPYFROMLIBDIR}/symbols/${EXE}
			  DESTINATION ${COPYTOLIBDIR}/symbols )
		endif()
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
	    file( COPY ${COPYFROMDATADIR}/rsm
		  DESTINATION ${COPYTODATADIR}/bin/${OD_PLFSUBDIR} )
	    file( COPY ${COPYFROMLIBDIR}/od_main_debug.bat
		  DESTINATION ${COPYTOLIBDIR} )
	endif()
    endif()
    foreach( EXTERNALLIB ${EXTERNALLIBS} )
	file( COPY ${COPYFROMLIBDIR}/${EXTERNALLIB}
	      DESTINATION ${COPYTOLIBDIR} )
    endforeach()
    set( EXTERNALLIBS "")

    ZIPPACKAGE( ${PACKAGE_FILENAME} ${REL_DIR} ${PACKAGE_DIR} )
endmacro( CREATE_PACKAGE )


macro( COPY_THIRDPARTYLIBS )
    list( APPEND SYSLIBS ${SYSTEMLIBS} )
    list( APPEND SSLLIBS ${OPENSSLLIBS} )
    foreach( LIB ${OD_THIRD_PARTY_FILES} )
	string( FIND ${LIB} "Qt" ISQTLIB )
	if (  APPLE AND NOT ${ISQTLIB} EQUAL -1 )
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
    set( RELFILENAM RELEASE )
    string( FIND ${PACKAGE_NAME} "dgb" STATUS )
    if( ${STATUS} EQUAL "0" )
	set( ODDGBSTR "dgb" )
	set( RELFILENAM ${RELFILENAM}.${ODDGBSTR}.txt )
	file( GLOB QMFILES ${COPYFROMDATADIR}/data/localizations/*.qm )
	foreach( QMFILE ${QMFILES} )
	    get_filename_component( QMFILENM ${QMFILE} NAME )
	    file( COPY ${COPYFROMDATADIR}/data/localizations/${QMFILENM}
		  DESTINATION ${COPYTODATADIR}/data/localizations )
	 endforeach()
    else()
	set( ODDGBSTR "od" )
	file( COPY ${COPYFROMDATADIR}/relinfo/README.txt
	      DESTINATION ${COPYTODATADIR}/relinfo )
	set( RELFILENAM ${RELFILENAM}.txt )
	file( COPY ${COPYFROMDATADIR}/relinfo/RELEASEINFO.txt
	      DESTINATION ${COPYTODATADIR}/doc/ReleaseInfo )
    endif()

    file( COPY ${COPYFROMDATADIR}/relinfo/${RELFILENAM}
	  DESTINATION ${COPYTODATADIR}/doc/ReleaseInfo )
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

    set( REL_DIR "${OpendTect_VERSION_MAJOR}.${OpendTect_VERSION_MINOR}.${OpendTect_VERSION_PATCH}" )
    set( FULLVER_NAME "${OpendTect_FULL_VERSION}" )
    if( APPLE )
	set( REL_DIR "OpendTect\ ${REL_DIR}.app/Contents" )
    endif()

    set( DESTINATION_DIR "${PACKAGE_DIR}/${REL_DIR}" )
    if( EXISTS ${DESTINATION_DIR} )
	file( REMOVE_RECURSE ${DESTINATION_DIR} )
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

#Genarate Symbols and then Strip the binaries
macro ( OD_GENERATE_BREAKPAD_SYMBOLS ALLLIBS EXECS)
    if ( NOT DEFINED BREAKPAD_DIR )
	message ( FATAL_ERROR "BREAKPAD_DIR not defined" )
    endif()
    if ( NOT EXISTS ${BREAKPAD_DIR} )
	message ( FATAL_ERROR "BREAKPAD_DIR: ${BREAKPAD_DIR} not found" )
    endif()

    set( SYMBOLDIRNM symbols_${OD_PLFSUBDIR}_${FULLVER_NAME} )
    if( NOT EXISTS ${PACKAGE_DIR}/${SYMBOLDIRNM} )
	file( MAKE_DIRECTORY ${PACKAGE_DIR}/symbols/${SYMBOLDIRNM} )
    endif()
    set( SYMBOLDIR ${PACKAGE_DIR}/symbols/${SYMBOLDIRNM} )

if( UNIX )
    set( LIBNAMES "" )
    foreach( LIB ${ALLLIBS} )
	set(SOLIB "lib${LIB}.so")
	set( LIBNAMES ${LIBNAMES} ${SOLIB} )
    endforeach()
    set( ALLLIBSBINS ${LIBNAMES} ${EXECS} )
    set( SYMGENCMD ${BREAKPAD_DIR}/bin/dump_syms )
elseif( WIN32 )
    set( LIBSBINS ${ALLLIBS} ${EXECS} )
    set( ALLLIBSBINS "" )
    foreach( LIBNM ${LIBSBINS} )
	if ( ${LIBNM} STREQUAL "lmutil" )
	    set( LIBNM "" )
	endif()
	set( ALLLIBSBINS ${ALLLIBSBINS} ${LIBNM} )
    endforeach()
    set( SYMGENCMD ${BREAKPAD_DIR}/bin/dump_syms.exe )
endif()

    foreach( FILENAME ${ALLLIBSBINS} )
	if ( UNIX )
	    execute_process( COMMAND "${SYMGENCMD}" ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Release/${FILENAME}
			     OUTPUT_FILE ${SYMBOLDIR}/${FILENAME}.sym )
	elseif( WIN32 )
	    execute_process( COMMAND "${SYMGENCMD}" ${BINARY_DIR}/bin/${OD_PLFSUBDIR}/Release/${FILENAME}.pdb
			     OUTPUT_FILE ${SYMBOLDIR}/${FILENAME}.sym )
	endif()

	file( STRINGS ${SYMBOLDIR}/${FILENAME}.sym STUFF LIMIT_COUNT 1 )
	string( REGEX REPLACE " " ";" NEWSTUFF ${STUFF} )
	set( WORDS ${NEWSTUFF} )
	list( GET WORDS 3 SYMADDRESS ) #assuming 4th word is symbol address
	if( UNIX )
	    file( MAKE_DIRECTORY ${SYMBOLDIR}/${FILENAME}/${SYMADDRESS} )
	elseif( WIN32 )
	    file( MAKE_DIRECTORY ${SYMBOLDIR}/${FILENAME}.pdb/${SYMADDRESS} )
	endif()

	if ( UNIX )
	    file( RENAME ${SYMBOLDIR}/${FILENAME}.sym ${SYMBOLDIR}/${FILENAME}/${SYMADDRESS}/${FILENAME}.sym )
	elseif( WIN32 )
	    file( RENAME ${SYMBOLDIR}/${FILENAME}.sym ${SYMBOLDIR}/${FILENAME}.pdb/${SYMADDRESS}/${FILENAME}.sym )
	endif()
    endforeach()
endmacro( OD_GENERATE_BREAKPAD_SYMBOLS )

