#(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
# Description:  CMake script to create installer
# Author:       Nageswara
# Date:		May 2013
#RCS:           $Id$

macro( init_destinationdir )
    if( UNIX )
	set( SHFILENAME "OpendTect_Installer_${OD_PLFSUBDIR}.sh" )
    elseif( APPLE )
	set( SHFILENAME "OpendTect\ Installer.dmg" )
    elseif( WIN32 )
	set( SHFILENAME "OpendTect_Installer_${OD_PLFSUBDIR}.exe" )
	set( SETUPDIR od_${OD_PLFSUBDIR}_setup )
    endif()

    if( NOT EXISTS ${PSD}/packages )
        file( MAKE_DIRECTORY ${PSD}/packages )
    endif()
    if( NOT EXISTS ${PSD}/packages/installers )
        file( MAKE_DIRECTORY ${PSD}/packages/installers )
    endif()

    set( PACKAGE_DIR ${PSD}/packages/installers)
    if( EXISTS ${PACKAGE_DIR}/${SHFILENAME} )
	file( REMOVE_RECURSE ${PACKAGE_DIR}/${SHFILENAME} )
	if( WIN32 )
	    file( REMOVE_RECURSE "${PACKAGE_DIR}/${SETUPDIR}.zip" )
	    file( REMOVE_RECURSE "${PACKAGE_DIR}/${SETUPDIR}" )
	endif()
    endif()

    if( UNIX OR WIN32 )
	set( REL_DIR OpendTect_Installer_${OD_PLFSUBDIR} )
    elseif( APPLE )
	set( REL_DIR "OpendTect\ Installer.app" )
    endif()

    set( VER_FILENAME "instmgr_${OD_PLFSUBDIR}" )
    set( FULLVER_NAME "${OpendTect_VERSION_MAJOR}.${OpendTect_VERSION_MINOR}.${OpendTect_VERSION_DETAIL}${Installer_VERSION}" )
    set( DESTINATION_DIR "${PACKAGE_DIR}/${REL_DIR}" )
    if( EXISTS ${DESTINATION_DIR} )
	file( REMOVE_RECURSE ${DESTINATION_DIR} )
    endif()

    file( MAKE_DIRECTORY ${DESTINATION_DIR} ${DESTINATION_DIR}/relinfo )
    file( WRITE ${DESTINATION_DIR}/relinfo/ver.${VER_FILENAME}.txt ${FULLVER_NAME} )
    file( APPEND ${DESTINATION_DIR}/relinfo/ver.${VER_FILENAME}.txt "\n" )
    if( APPLE )
    endif()
    if( WIN32 )
	file( MAKE_DIRECTORY ${PACKAGE_DIR}/${SETUPDIR} )
    endif()
endmacro( init_destinationdir )

set( MAKESELF_DIR ${PSD}/data/install_files/unixscripts/makeself )
if( NOT DEFINED MAKESELF_DIR )
    message( FATAL_ERROR "MAKESELF_DIR is not set" )
endif()

if( NOT EXISTS "${MAKESELF_DIR}/makeself.sh" )
    message( FATAL_ERROR "makeself.sh script does not exist in ${MAKESELF_DIR}" )
endif()
if( NOT EXISTS "${MAKESELF_DIR}/makeself-header.sh" )
    message( FATAL_ERROR "makeself-header.sh script does not exist in ${MAKESELF_DIR}" )
endif()

include( CMakeModules/packagescripts/installerdefs.cmake )
init_destinationdir()

if( APPLE )
    set( MACBINDIR "Contents/MacOS" )
    file( MAKE_DIRECTORY ${DESTINATION_DIR}/Contents
	  		 ${DESTINATION_DIR}/Contents/Resources
			 ${DESTINATION_DIR}/Contents/MacOS )
endif()

set( COPYTODIR ${DESTINATION_DIR} )
file( MAKE_DIRECTORY ${DESTINATION_DIR}/data
		     ${DESTINATION_DIR}/data/icons.Default)
foreach( DATAFILE ${DATAFILES} )
    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
		     "${CMAKE_INSTALL_PREFIX}/data/icons.Default/${DATAFILE}"
		     ${DESTINATION_DIR}/data/icons.Default )
endforeach()

file( MAKE_DIRECTORY ${DESTINATION_DIR}/doc ${DESTINATION_DIR}/doc/User
		     ${DESTINATION_DIR}/doc/User/base )
execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${CMAKE_INSTALL_PREFIX}/doc/od_WindowLinkTable.txt
			 ${DESTINATION_DIR}/doc/User/base/WindowLinkTable.txt )

message( "Copying ${OD_PLFSUBDIR} libraries" )
foreach( FILE ${LIBLIST} )
    if( ${OD_PLFSUBDIR} STREQUAL "lux64" OR ${OD_PLFSUBDIR} STREQUAL "lux32" )
	    set(LIB "lib${FILE}.so")
    elseif( APPLE )
	    set( LIB "lib${FILE}.dylib" )
    elseif( WIN32 )
	    set( LIB "${FILE}.dll" )
    endif()

    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
		     ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Release/${LIB}
		     ${DESTINATION_DIR} )
    if( WIN32 )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Release/${LIB}
			 ${PACKAGE_DIR}/${SETUPDIR} )
    endif()
endforeach()

foreach( QFILE ${THIRDPARTYLIBS} )
    foreach( QNAME ${OD_THIRD_PARTY_LIBS} )
	string(FIND ${QNAME} ${QFILE} VALUE )
	if( ${VALUE} GREATER -1 )
	    set( QLIB ${QNAME} )
	endif()
    endforeach()
    
    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
		     ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Release/${QLIB}
		     ${DESTINATION_DIR} )
    if( WIN32 )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Release/${QLIB}
			 ${PACKAGE_DIR}/${SETUPDIR} )
    endif()
endforeach()


message( "Copying ${OD_PLFSUBDIR} executables" )
foreach( EXE ${EXECLIST} )
message( "Copying executables" )
    if( WIN32 )
	    set( EXE "${EXE}.exe" )
    endif()

    execute_process( COMMAND ${CMAKE_COMMAND} -E copy
		     ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Release/${EXE}
		     ${DESTINATION_DIR} )
endforeach()

if( WIN32 )
    foreach( SETUPEXE ${SETUPEXECLIST} )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${CMAKE_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/Release/${SETUPEXE}.exe
			 ${PACKAGE_DIR}/${SETUPDIR} )
    endforeach()
endif()

foreach( ODSCRIPT ${ODSCRIPTS} )
    if( UNIX )
	execute_process( COMMAND ${CMAKE_COMMAND} -E copy
			 ${CMAKE_INSTALL_PREFIX}/${ODSCRIPT} ${DESTINATION_DIR} )
    endif()
endforeach()

if( UNIX)
    execute_process( COMMAND ${MAKESELF_DIR}/makeself.sh ${DESTINATION_DIR} ${PACKAGE_DIR}/${SHFILENAME} \"InstallManager\" ./run_installer )

elseif( WIN32 )
    file( GLOB ZIPFILES ${DESTINATION_DIR}/* )
    foreach( ZIPFILE ${ZIPFILES} )
	get_filename_component( LIB ${ZIPFILE} NAME )
	execute_process( COMMAND ${CMAKE_COMMAND} -E chdir ${PACKAGE_DIR}/${REL_DIR}
		         zip -r -q ${PACKAGE_DIR}/${REL_DIR}.zip ${LIB} 
			 RESULT_VARIABLE STATUS )
    endforeach()
    if( NOT ${STATUS} EQUAL "0" )
	message( "Failed to create zip file ${REL_DIR}.zip" )
    endif()

    execute_process( COMMAND "C:\Program Files (x86)\WinZip Self-Extractor\WZIPSE32.EXE" ${REL_DIR}.zip  -silent -setup /auto -runasadmin -c od_instmgr.exe
		     WORKING_DIRECTORY ${PACKAGE_DIR}
		     RESULT_VARIABLE STATUS )
    if( NOT ${STATUS} EQUAL "0" )
	message( "Failed to create ${OD_PLFSUBDIR} self extractor file ${SHFILENAME} " )
    endif()

    execute_process( COMMAND ${PSD}/bin/win64/zip -r -q ${SETUPDIR}.zip ${SETUPDIR}
		     WORKING_DIRECTORY ${PACKAGE_DIR}
		     RESULT_VARIABLE STATUS )
    if( NOT ${STATUS} EQUAL "0" )
	message( "Failed to create ${SETUPDIR}.zip file " )
    endif()

endif()

