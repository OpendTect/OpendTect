#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id: ODInternal.cmake,v 1.6 2012/09/11 06:14:31 cvsnageswara Exp $
#_______________________________________________________________________________

#Install cmake things.
install ( DIRECTORY CMakeModules DESTINATION .
	  PATTERN ".svn" EXCLUDE )

#Install plugin example
install( DIRECTORY ${CMAKE_SOURCE_DIR}/doc/Programmer/pluginexample
	 	   DESTINATION doc/Programmer
	 	   PATTERN ".svn" EXCLUDE )

#Install batchprogram example
install( DIRECTORY ${CMAKE_SOURCE_DIR}/doc/Programmer/batchprogexample
		   DESTINATION doc/Programmer
		   PATTERN ".svn" EXCLUDE )

#install data folder
install( DIRECTORY ${CMAKE_SOURCE_DIR}/data 
	 DESTINATION .
	 PATTERN "install_files" EXCLUDE
	 PATTERN ".svn" EXCLUDE )

install( DIRECTORY ${CMAKE_SOURCE_DIR}/relinfo 
	 DESTINATION .
	 PATTERN ".svn" EXCLUDE )
install( FILES ${CMAKE_SOURCE_DIR}/doc/ReleaseInfo/RELEASE.txt
	 DESTINATION relinfo )
install( FILES ${CMAKE_SOURCE_DIR}/doc/ReleaseInfo/RELEASEINFO.txt
	 DESTINATION relinfo )
IF( WIN32 )
    install( DIRECTORY ${CMAKE_SOURCE_DIR}/bin/win32/rsm 
	     DESTINATION .
	     PATTERN ".svn" EXCLUDE )
ENDIF()

#install scripts
install( DIRECTORY ${CMAKE_SOURCE_DIR}/doc/Scripts 
	 DESTINATION doc
	 PATTERN ".svn" EXCLUDE )

install( FILES ${CMAKE_SOURCE_DIR}/doc/about.html
	       DESTINATION doc )

if ( UNIX )
    file ( GLOB TEXTFILES
	   ${CMAKE_SOURCE_DIR}/data/install_files/unixscripts/*.txt )
    file( GLOB PROGRAMS
	  ${CMAKE_SOURCE_DIR}/data/install_files/unixscripts/* )
    list ( REMOVE_ITEM PROGRAMS
	   ${CMAKE_SOURCE_DIR}/data/install_files/unixscripts/.svn )
    foreach ( TEXTFILE ${TEXTFILES} )
	list ( REMOVE_ITEM PROGRAMS ${TEXTFILE} )
    endforeach()

    install ( PROGRAMS ${PROGRAMS} DESTINATION . )
    install ( FILES ${TEXTFILES} DESTINATION . )
endif( UNIX )

if ( APPLE )
install( DIRECTORY ${CMAKE_SOURCE_DIR}/data/install_files/macscripts/Contents 
	 DESTINATION .
	 PATTERN ".svn" EXCLUDE )
endif( APPLE )

SET( QJPEG ${QT_QJPEG_PLUGIN_RELEASE} )
SET( LMHOSTID lmhostid)
IF( WIN32 )
    install ( PROGRAMS ${CMAKE_SOURCE_DIR}/bin/win/unzip.exe DESTINATION
	      ${CMAKE_INSTALL_PREFIX}/${OD_EXEC_OUTPUT_RELPATH} )
    INSTALL( PROGRAMS "C:/Program\ Files \(x86\)/Microsoft\ SDKs/Windows/v7.0A/Bin/signtool.exe" DESTINATION ${CMAKE_INSTALL_PREFIX}/${OD_EXEC_OUTPUT_RELPATH} )
    SET( QJPEG ${QTDIR}/plugins/imageformats/qjpeg4.dll )
    IF( ${OD_PLFSUBDIR} STREQUAL "win32" )
#	SET( MSVCPATH "C:/Program\ Files/Microsoft\ Visual\ Studio\ 10.0/VC/redist/x86/Microsoft.VC100.CRT" )
	SET( MSVCPATH "C:/Program\ Files \(x86\)/Microsoft\ Visual\ Studio\ 10.0/VC/redist/x86/Microsoft.VC100.CRT" )
    ELSEIF( ${OD_PLFSUBDIR} STREQUAL "win64" )
	SET( MSVCPATH "C:/Program\ Files \(x86\)/Microsoft\ Visual\ Studio\ 10.0/VC/redist/x64/Microsoft.VC100.CRT" )
    ENDIF()
    SET( LMHOSTID "lmhostid.exe" )
ENDIF()

install ( PROGRAMS ${QJPEG} DESTINATION imageformats )
install ( PROGRAMS ${CMAKE_SOURCE_DIR}/bin/${OD_PLFSUBDIR}/${LMHOSTID}
	    DESTINATION ${CMAKE_INSTALL_PREFIX}/${OD_EXEC_OUTPUT_RELPATH} )
IF( EXISTS ${MSVCPATH} )
        FILE( GLOB MSVCDLLS ${MSVCPATH}/*.dll )
        FOREACH( DLL ${MSVCDLLS} )
	    INSTALL( FILES ${DLL} DESTINATION bin/${OD_PLFSUBDIR}/Release )
        ENDFOREACH()
ENDIF()

add_custom_target( sources ${CMAKE_COMMAND}
	-DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
	-DOD_PLFSUBDIR=${OD_PLFSUBDIR}
	-P ${CMAKE_SOURCE_DIR}/CMakeModules/ODInstallSources.cmake 
	 COMMENT "Installing sources" )
include ( ODSubversion )
add_custom_target( docpackages ${CMAKE_COMMAND}
	-DOpendTect_VERSION_MAJOR=${OpendTect_VERSION_MAJOR}
	-DOpendTect_VERSION_MINOR=${OpendTect_VERSION_MINOR}
	-DOpendTect_VERSION_DETAIL=${OpendTect_VERSION_DETAIL}
	-DOpendTect_VERSION_PATCH=${OpendTect_VERSION_PATCH}
	-DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
	-DOD_PLFSUBDIR=${OD_PLFSUBDIR}
	-DPSD=${PROJECT_SOURCE_DIR}
	-P ${PROJECT_SOURCE_DIR}/CMakeModules/packagescripts/ODMakeDocPackages.cmake
	 COMMENT "Preparing doc packages" )

OD_CURRENT_YEAR( YEAR )
IF( APPLE )
    string(REPLACE "\n" "" YEAR ${YEAR} )
    configure_file( ${CMAKE_SOURCE_DIR}/CMakeModules/templates/Info.plist.in
		    ${CMAKE_SOURCE_DIR}/data/install_files/macscripts/Contents/Info.plist @ONLY )

ENDIF()
