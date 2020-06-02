#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:	CMake script to build a release
# Author:	Nageswara
# Date:		August 2012		
#RCS:		$Id$

#TODO Change macro names to CAPITAL letters.

macro ( create_package PACKAGE_NAME )
    if( ${PACKAGE_NAME} STREQUAL "base" )
	if( APPLE )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${COPYFROMDATADIR}/qt_menu.nib
			     ${COPYTODATADIR}/qt_menu.nib )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			     ${COPYFROMDATADIR}/od.icns
			     ${COPYTODATADIR}/. )
	endif()


	if( NOT MATLAB_DIR STREQUAL "" )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/MATLAB
			     ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/MATLAB )
	endif()

	copy_thirdpartylibs()
	set( LIBLIST ${LIBLIST};${PLUGINS};osgGeo )
    endif()

    if ( SYSTEMLIBS )
        if( ${OD_PLFSUBDIR} STREQUAL "lux64" )
            copy_unix_systemlibs()
            unset( SYSTEMLIBS )
        endif()
    endif()
    if ( PYTHONDIR )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			 ${CMAKE_INSTALL_PREFIX}/bin/${PYTHONDIR}
			 ${DESTINATION_DIR}/bin/python )
	unset( PYTHONDIR  )
    endif()

    message( "Copying ${OD_PLFSUBDIR} libraries" )
    foreach( FILE ${LIBLIST} )
	string( FIND ${FILE} "osgGeo" ISOSGGEO )
	if( ${OD_PLFSUBDIR} STREQUAL "lux64" OR ${OD_PLFSUBDIR} STREQUAL "lux32" )
		set(LIB "lib${FILE}.so")
	elseif( APPLE )
		set( LIB "lib${FILE}.dylib" )
	elseif( WIN32 )
		set( LIB "${FILE}.dll" )
	endif()

	if (  NOT ${ISOSGGEO} EQUAL -1 )
	   #Added osgGeo lib to OD_THIRD_PARTY_LIBS. Will be used to create devel package
	    set( OD_THIRD_PARTY_LIBS ${OD_THIRD_PARTY_LIBS} ${LIB} )
	endif()

	if( WIN64 )
	    #Stripping not required on windows
	elseif( APPLE )
	    #Not using breakpad on MAC
	else()
	    execute_process( COMMAND strip ${COPYFROMLIBDIR}/${LIB} )
	endif()
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${COPYFROMLIBDIR}/${LIB}
			 ${COPYTOLIBDIR}/${LIB} )
#copying breakpad symbols
	if ( OD_ENABLE_BREAKPAD )
	    if( WIN32 )
		#TODO
	    elseif( APPLE )
		#TODO
	    else()
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
				 ${COPYFROMLIBDIR}/symbols/${LIB}
				 ${COPYTOLIBDIR}/symbols/${LIB} )
	    endif()
	endif()

	file( GLOB ALOFILES ${COPYFROMDATADIR}/plugins/${OD_PLFSUBDIR}/*.${FILE}.alo )
	foreach( ALOFILE ${ALOFILES} )
	    get_filename_component( ALOFILENAME ${ALOFILE} NAME )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${ALOFILE}
				     ${COPYTODATADIR}/plugins/${OD_PLFSUBDIR}/${ALOFILENAME} )
	endforeach()
    endforeach()

    if( ${PACKAGE_NAME} STREQUAL "dgbbase" OR ${PACKAGE_NAME} STREQUAL "v7" )
#Install lm 
	foreach( SPECFILE ${SPECFILES} )
	     execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			      ${COPYFROMDATADIR}/${SPECFILE}
			      ${COPYTODATADIR}/. )
	endforeach()
        if ( ${PACKAGE_NAME} STREQUAL "v7" )
	    foreach( FILES ${v7SCRIPTS} )
		 file( GLOB SCRIPTS ${COPYFROMDATADIR}/bin/${FILES} )
		 foreach( SCRIPT ${SCRIPTS} )
		    execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${SCRIPT}
					     ${COPYTODATADIR}/bin/. )
		 endforeach()
	    endforeach()
	endif()

	execute_process( COMMAND ${CMAKE_COMMAND} -E
			 copy_directory ${COPYFROMDATADIR}/bin/${OD_PLFSUBDIR}/lm.dgb
			 ${COPYTODATADIR}/bin/${OD_PLFSUBDIR}/lm.dgb )
	if( UNIX )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			     ${COPYFROMDATADIR}/mk_flexlm_links.csh
			     ${COPYTODATADIR}/bin/${OD_PLFSUBDIR}/lm.dgb )
	    execute_process( COMMAND
		    ${COPYTODATADIR}/bin/${OD_PLFSUBDIR}/lm.dgb/mk_flexlm_links.csh
		    WORKING_DIRECTORY ${COPYTODATADIR}/bin/${OD_PLFSUBDIR}/lm.dgb
		    RESULT_VARIABLE STATUS )
	    file( REMOVE_RECURSE ${COPYTODATADIR}/bin/${OD_PLFSUBDIR}/lm.dgb/mk_flexlm_links.csh )
	    if( NOT ${STATUS} EQUAL "0" )
		message( "Failed to create license related links" )
	    endif()
	endif()	
        
        if ( PYTHONDIR )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${CMAKE_INSTALL_PREFIX}/bin/${PYTHONDIR}
			     ${DESTINATION_DIR}/bin/python )
	    unset( PYTHONDIR  )
        endif()
    endif()

    if( WIN32 )
	if( ${PACKAGE_NAME} STREQUAL "base" )
		set( EXECLIST "${EXECLIST};${WINEXECLIST}" )
	endif()
    endif()

    message( "Copying ${OD_PLFSUBDIR} executables" )
    foreach( EXE ${EXECLIST} )
	if( WIN64 )
	    #Stripping not required on windows
	elseif( APPLE )
	    #Not using breakpad on MAC , Stripping not required
	else()
	    execute_process( COMMAND strip ${COPYFROMLIBDIR}/${EXE} )
	endif()

	if( WIN32 )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			     ${COPYFROMLIBDIR}/${EXE}.exe
			     ${COPYTOLIBDIR}/${EXE}.exe )
	elseif( APPLE )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			     ${COPYFROMEXEDIR}/${EXE}
			     ${COPYTOEXEDIR}/${EXE} )

	else()
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			     ${COPYFROMLIBDIR}/${EXE}
			     ${COPYTOLIBDIR}/${EXE} )
	endif()
#copying breakpad symbols
	if ( OD_ENABLE_BREAKPAD )
	    if ( WIN32 )
		#TODO
	    elseif( APPLE )
		#TODO
	    else()
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
				 ${COPYFROMLIBDIR}/symbols/${EXE}
				 ${COPYTOLIBDIR}/symbols/${EXE} )
	    endif()
	endif()
    endforeach()

    if( ${PACKAGE_NAME} STREQUAL "base" )
	foreach( SPECFILE ${SPECFILES} )
	     execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			      ${COPYFROMDATADIR}/${SPECFILE}
			      ${COPYTODATADIR}/. )
	endforeach()
	foreach( FILES ${ODSCRIPTS} )
	     file( GLOB SCRIPTS ${COPYFROMDATADIR}/bin/${FILES} )
	     foreach( SCRIPT ${SCRIPTS} )
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${SCRIPT}
					 ${COPYTODATADIR}/bin/. )
	     endforeach()
	endforeach()

	if( WIN32 )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
				     ${COPYFROMDATADIR}/rsm
				     ${COPYTODATADIR}/bin/${OD_PLFSUBDIR}/rsm )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			     ${COPYFROMLIBDIR}/od_main_debug.bat
			     ${COPYTOLIBDIR}/od_main_debug.bat )
	endif()
    endif()
    foreach( EXTERNALLIB ${EXTERNALLIBS} )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${COPYFROMLIBDIR}/${EXTERNALLIB}
			 ${COPYTOLIBDIR}/${EXTERNALLIB} )
    endforeach()
    set( EXTERNALLIBS "")

    zippackage( ${PACKAGE_FILENAME} ${REL_DIR} ${PACKAGE_DIR} )
    message( "DONE" )
endmacro( create_package )


macro( copy_thirdpartylibs )
    message( "Copying ${OD_PLFSUBDIR} thirdparty libraries" )
    foreach( LIB ${OD_THIRD_PARTY_LIBS} )
	if ( OD_ENABLE_BREAKPAD )
	    if ( WIN32 )
		#TODO Not including symbols on windows platform
	    elseif ( APPLE )
		#TODO
	    else()
		execute_process( COMMAND strip ${COPYFROMLIBDIR}/${LIB} )
		if ( EXISTS ${COPYFROMLIBDIR}/symbols/${LIB} ) #including thirdparty symbols if present
		    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
				     ${COPYFROMLIBDIR}/symbols/${LIB}
				     ${COPYTOLIBDIR}/symbols/${LIB} )
		endif()
	    endif()
	endif()


	string( FIND ${LIB} "Qt" ISQTLIB )
	string( FIND ${LIB} "osg" ISOSGLIB )
	#Checking ISOSGLIB value to avoid OSG library libosgQt.ylib
	if ( ${QT_VERSION_MAJOR} STREQUAL "${QT_VERSION_MAJOR}" AND APPLE
	     AND NOT ${ISQTLIB} EQUAL -1  AND ${ISOSGLIB} EQUAL -1 )
	    file( MAKE_DIRECTORY ${COPYTOLIBDIR}/${LIB}.framework
				 ${COPYTOLIBDIR}/${LIB}.framework/Versions
				 ${COPYTOLIBDIR}/${LIB}.framework/Versions/${QT_VERSION_MAJOR} )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${COPYFROMLIBDIR}/${LIB}
			     ${COPYTOLIBDIR}/${LIB}.framework/Versions/${QT_VERSION_MAJOR} )
	else()
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${COPYFROMLIBDIR}/${LIB} ${COPYTOLIBDIR} )
	endif()

    endforeach()

    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
		     ${COPYFROMLIBDIR}/../translations
		     ${COPYTODATADIR}/bin/${OD_PLFSUBDIR}/translations )
    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
		     ${COPYFROMLIBDIR}/iconengines
		     ${COPYTODATADIR}/bin/${OD_PLFSUBDIR}/Release/iconengines )
    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
		     ${COPYFROMLIBDIR}/imageformats
		     ${COPYTODATADIR}/bin/${OD_PLFSUBDIR}/Release/imageformats )
    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
		     ${COPYFROMLIBDIR}/platforms
		     ${COPYTODATADIR}/bin/${OD_PLFSUBDIR}/Release/platforms )
    if ( UNIX )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			 ${COPYFROMLIBDIR}/xcbglintegrations
			 ${COPYTODATADIR}/bin/${OD_PLFSUBDIR}/Release/xcbglintegrations )
    endif()
    if ( EXISTS ${COPYFROMLIBDIR}/../resources )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			 ${COPYFROMLIBDIR}/../resources
			 ${COPYTODATADIR}/bin/${OD_PLFSUBDIR}/resources )
    endif()
endmacro( copy_thirdpartylibs )

macro( PREPARE_WIN_THIRDPARTY_DEBUGLIST DEBUGFILELIST)
    if( WIN32 )
	foreach( THIRDPARTYDLL ${OD_THIRD_PARTY_LIBS} )
	    get_filename_component(FILENM ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Release/${THIRDPARTYDLL} NAME_WE)
	    string( FIND ${FILENM} "5" ISQTFILE )
	    string( FIND ${FILENM} "osg" ISOSGFILE )
	    string( FIND ${FILENM} "OpenThreads" OSGOTFILE )
	if ( NOT "${ISQTFILE}" EQUAL -1 ) #  File is Qt
	    string(REGEX REPLACE "${FILENM}" "${FILENM}d" QTDEBUGFILENAME ${FILENM} )
	    list(APPEND ${DEBUGFILELIST} ${QTDEBUGFILENAME}.dll )
	elseif ( (NOT "${ISOSGFILE}" EQUAL -1) OR (NOT "${OSGOTFILE}" EQUAL -1) ) # File is osg
	    string(REGEX REPLACE "${FILENM}" "${FILENM}d" OSGDEBUGFILENAME ${FILENM} )
	    list(APPEND ${DEBUGFILELIST} ${OSGDEBUGFILENAME}.dll )
	endif()
	endforeach()
	list(REMOVE_DUPLICATES ${DEBUGFILELIST} )
    endif()
endmacro()


macro( copy_unix_systemlibs )
    message( "Copying ${OD_PLFSUBDIR} system libraries" )
    file( MAKE_DIRECTORY ${COPYTOLIBDIR}/systemlibs )
    if( ${OD_PLFSUBDIR} STREQUAL "lux64" )
	foreach( SYSLIB ${SYSTEMLIBS} )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${COPYFROMLIBDIR}/${SYSLIB} ${COPYTOLIBDIR}/systemlibs/ )
	endforeach()
    endif()
endmacro()

macro( create_basepackages PACKAGE_NAME )
    if( ${PACKAGE_NAME} STREQUAL "basedata" OR ${PACKAGE_NAME} STREQUAL "v7basedatadefs" )
       execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			${COPYFROMDATADIR}/doc/system_requirements.html
			${COPYTODATADIR}/doc/system_requirements.html )
       execute_process( COMMAND ${CMAKE_COMMAND} -E copy
				${COPYFROMDATADIR}/relinfo/RELEASE.txt
				${COPYTODATADIR}/doc/ReleaseInfo/RELEASE.txt )
       execute_process( COMMAND ${CMAKE_COMMAND} -E copy
				${COPYFROMDATADIR}/relinfo/RELEASEINFO.txt
				${COPYTODATADIR}/doc/ReleaseInfo/RELEASEINFO.txt )
	foreach( LIBS ${DATALIBLIST} )
	    file( GLOB DATAFILES ${COPYFROMDATADIR}/data/${LIBS} )
	    foreach( DATA ${DATAFILES} )
    #TODO if possible copy files instead of INSTALL
		  file( INSTALL DESTINATION ${COPYTODATADIR}/data
				TYPE DIRECTORY FILES ${DATA}
				REGEX ".svn" EXCLUDE )
	    endforeach()
	endforeach()
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${COPYFROMDATADIR}/relinfo/README.txt
			 ${COPYTODATADIR}/relinfo/README.txt )
   endif()
   if( ${PACKAGE_NAME} STREQUAL "dgbbasedata" )
       execute_process( COMMAND ${CMAKE_COMMAND} -E copy
				${COPYFROMDATADIR}/relinfo/RELEASE.dgb.txt
				${COPYTODATADIR}/doc/ReleaseInfo/RELEASE.dgb.txt )
       foreach( DATALIB ${DATALIBLIST} )
	  if( IS_DIRECTORY "${COPYFROMDATADIR}/data/${DATALIB}" )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${COPYFROMDATADIR}/data/${DATALIB}
			     ${COPYTODATADIR}/data/${DATALIB} )
	  else()
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			     ${COPYFROMDATADIR}/data/${DATALIB}
			     ${COPYTODATADIR}/data/${DATALIB} )
	  endif()
       endforeach()
       file( GLOB QMFILES ${COPYFROMDATADIR}/data/translations/*.qm )
	foreach( QMFILE ${QMFILES} )
	    get_filename_component( QMFILENM ${QMFILE} NAME )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			     ${COPYFROMDATADIR}/data/translations/${QMFILENM}
			     ${COPYTODATADIR}/data/translations/${QMFILENM} )
	endforeach()
   endif()
if ( NOT ${PACKAGE_NAME}  STREQUAL "v7basedatadefs" )
    zippackage( ${PACKAGE_FILENAME} ${REL_DIR} ${PACKAGE_DIR} )
endif()
endmacro( create_basepackages )


macro( init_destinationdir  PACKAGE_NAME )
    set ( PACKAGE_FILENAME ${PACKAGE_NAME} )
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

    if( EXISTS ${PACKAGE_DIR}/${PACKAGE_FILENAME} )
	file( REMOVE_RECURSE ${PACKAGE_DIR}/${PACKAGE_FILENAME} )
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
	file( MAKE_DIRECTORY ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/Release )
	set( COPYFROMLIBDIR ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Release )
	set( COPYTOLIBDIR ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/Release )
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

    file( WRITE ${COPYTODATADIR}/relinfo/ver.${VER_FILENAME}.txt ${FULLVER_NAME} )
    file( APPEND ${COPYTODATADIR}/relinfo/ver.${VER_FILENAME}.txt "\n" )
    if( APPLE )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			     ${CMAKE_INSTALL_PREFIX}/Contents/Info.plist
			     ${DESTINATION_DIR}/. )
    endif()

    message( "Preparing package ${VER_FILENAME}.zip ......" )
endmacro( init_destinationdir )


macro( create_develpackages )
    file( MAKE_DIRECTORY ${COPYFROMDATADIR}/doc
			 ${COPYTODATADIR}/doc/Programmer)
    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
		     ${SOURCE_DIR}/CMakeLists.txt ${COPYTODATADIR} )
    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
		     ${COPYFROMDATADIR}/doc/Programmer/batchprogexample
		     ${COPYTODATADIR}/doc/Programmer/batchprogexample )
    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
		     ${COPYFROMDATADIR}/doc/Programmer/pluginexample
		     ${COPYTODATADIR}/doc/Programmer/pluginexample )
    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
		     ${COPYFROMDATADIR}/dtect
		     ${COPYTODATADIR}/dtect )
    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
		     ${COPYFROMLIBDIR}/../Debug/platforms
		     ${COPYTODATADIR}/bin/${OD_PLFSUBDIR}/Debug/platforms )
    foreach( SPECFILE ${SPECFILES} )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${COPYFROMDATADIR}/doc/Programmer/${SPECFILE}
			 ${COPYTODATADIR}/doc/Programmer )
    endforeach()

    file( GLOB HTMLFILES ${BINARY_DIR}/doc/Programmer/*.html )
    foreach( HTMLFILE ${HTMLFILES} )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${HTMLFILE} ${COPYTODATADIR}/doc/Programmer )
    endforeach()
    file( GLOB PNGFILES ${SOURCE_DIR}/doc/Programmer/*.png )
    foreach( PNGFILE ${PNGFILES} )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${PNGFILE} ${COPYTODATADIR}/doc/Programmer )
    endforeach()

    foreach( DIR CMakeModules include src plugins spec )
	message( "Copying ${DIR} files" )
	if( "${DIR}" STREQUAL "plugins" )
	    foreach( ODPLUGIN ${ODPLUGINS} )
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
				 ${COPYFROMDATADIR}/plugins/${ODPLUGIN}
				 ${COPYTODATADIR}/plugins/${ODPLUGIN} )
	    endforeach()
	else()
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${COPYFROMDATADIR}/${DIR}
			     ${COPYTODATADIR}/${DIR} )
	endif()
    endforeach()

    if ( APPLE )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${COPYFROMDATADIR}/bin/od_cr_dev_env
			 ${COPYTODATADIR}/od_cr_dev_env )
    endif()

    if( WIN32 )
	file( MAKE_DIRECTORY ${DESTINATION_DIR}/bin
			     ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}
			     ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/Debug
			     ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/Release )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${SOURCE_DIR}/bin/od_cr_dev_env.bat
			 ${DESTINATION_DIR}/bin )

	set( DEVELLIBS ${ODLIBLIST} ${ODPLUGINS} ${SPECSOURCES} )
	#Copying dll, pdb and lib files.
	foreach( DLIB ${DEVELLIBS} )
	    file( GLOB FILES ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Debug/${DLIB}.* )
	    string( TOLOWER ${DLIB} PDBLIB )
	    file( GLOB PDBFILES ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Debug/${PDBLIB}.* )
	    set( FILES ${FILES} ${PDBFILES} )
	    foreach( FIL ${FILES} )
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			${FIL} ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/Debug )
	    endforeach()

	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
		${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Release/${DLIB}.lib
		${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/Release )
	endforeach()
	#Copying executables and pdb files
	foreach( EXELIB ${EXECLIST} )
	    file( GLOB EXEFILES ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Debug/${EXELIB}.* )
	    string( TOLOWER ${EXELIB} EXEPDBLIB )
	    file( GLOB EXEPDBFILES ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Debug/${EXEPDBLIB}.* )
	    set( EXEILES ${EXEFILES} ${EXEPDBFILES} )
	    foreach( ELIB ${EXEILES} )
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			${ELIB} ${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/Debug )
	    endforeach()
	endforeach()

	#Copying third party debug libraries
	PREPARE_WIN_THIRDPARTY_DEBUGLIST( DEBUGLIST )
	foreach( TLIB ${DEBUGLIST} )
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy
				${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Debug/${TLIB}
				${DESTINATION_DIR}/bin/${OD_PLFSUBDIR}/Debug )
	endforeach()
    endif()

    zippackage( ${PACKAGE_FILENAME} ${REL_DIR} ${PACKAGE_DIR} )
endmacro( create_develpackages )

macro( create_docpackages PACKAGE_NAME )
    if( WIN32 )
	if( ${PACKAGE_NAME} STREQUAL "doc" )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${CMAKE_INSTALL_PREFIX}/doc/od_userdoc
			     ${DESTINATION_DIR}/doc/od_userdoc )
	elseif( ${PACKAGE_NAME} STREQUAL "dgbdoc" )
	    execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
			     ${CMAKE_INSTALL_PREFIX}/doc/dgb_userdoc
			     ${DESTINATION_DIR}/doc/dgb_userdoc )
	    file( GLOB FILES ${CMAKE_INSTALL_PREFIX}/doc/flexnet* )
	    foreach( FIL ${FILES} )
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy ${FIL} ${DESTINATION_DIR}/doc )
	    endforeach()
	endif()
    else()
	if( ${PACKAGE_NAME} STREQUAL "classdoc" )
	    if( EXISTS ${CMAKE_INSTALL_PREFIX}/doc/Programmer/Generated )
		execute_process( COMMAND ${CMAKE_COMMAND} -E copy_directory
				 ${CMAKE_INSTALL_PREFIX}/doc/Programmer/Generated
				 ${DESTINATION_DIR}/doc/Programmer/Generated )
	    else()
		message( "Class doc not installed correctly. ${PACKAGE_FILENAME} is not self contained." )
	    endif()
	endif()
    endif()
    zippackage( ${PACKAGE_FILENAME} ${REL_DIR} ${PACKAGE_DIR} )
endmacro( create_docpackages )

macro( zippackage PACKAGE_FILENAME REL_DIR PACKAGE_DIR )
    if( WIN32 )
	message( "Using ${OD_PLFSUBDIR} zip command" )
	execute_process( COMMAND ${SOURCE_DIR}/bin/win64/zip -r -q
				 "${PACKAGE_FILENAME}" ${REL_DIR}
				 WORKING_DIRECTORY ${PACKAGE_DIR}
				 RESULT_VARIABLE STATUS )
    else()
	message( "Using ${OD_PLFSUBDIR} zip command" )
	execute_process( COMMAND zip -r -y -q "${PACKAGE_FILENAME}" ${REL_DIR} 
				 WORKING_DIRECTORY ${PACKAGE_DIR}
				 RESULT_VARIABLE STATUS )
    endif()

    if( NOT ${STATUS} EQUAL "0" )
	message( FATAL_ERROR "Failed to create zip file ${PACKAGE_FILENAME}" )
   endif()
endmacro( zippackage )

#Genarate Symbols and then Strip the binaries
macro ( OD_GENERATE_BREAKPAD_SYMBOLS ALLLIBS EXECS)
    if ( NOT DEFINED BREAKPAD_DIR )
	message ( FATAL_EEROR "BREAKPAD_DIR not defined" )
    endif()
    if ( NOT EXISTS ${BREAKPAD_DIR} )
	message ( FATAL_EEROR "BREAKPAD_DIR: ${BREAKPAD_DIR} not found" )
    endif()

    set( SYMBOLDIRNM symbols_v7_${OD_PLFSUBDIR}_${FULLVER_NAME} )
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
	    execute_process( COMMAND "${SYMGENCMD}" ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Release/${FILENAME} OUTPUT_FILE ${SYMBOLDIR}/${FILENAME}.sym )
	elseif( WIN32 )
	    execute_process( COMMAND "${SYMGENCMD}" ${BINARY_DIR}/bin/${OD_PLFSUBDIR}/Release/${FILENAME}.pdb OUTPUT_FILE ${SYMBOLDIR}/${FILENAME}.sym )
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
	if( UNIX )
	    execute_process( COMMAND strip ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Release/${FILENAME} )
	endif()
    endforeach()
endmacro() #OD_GENERATE_BREAKPAD_SYMBOLS

macro( create_flexnet_pkg )
    file( GLOB FLEXBINS ${COPYFROMDATADIR}/bin/${OD_PLFSUBDIR}/lm.dgb/* )
    foreach( FLEXBIN ${FLEXBINS} )
	get_filename_component( BINNAME ${FLEXBIN} NAME )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${FLEXBIN}
			 ${COPYTODATADIR}/bin/${OD_PLFSUBDIR}/lm.dgb/${BINNAME} )
    endforeach()

    zippackage( ${PACKAGE_FILENAME} ${REL_DIR} ${PACKAGE_DIR} )
endmacro( create_flexnet_pkg )
