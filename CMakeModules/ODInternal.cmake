#_______________________Pmake__________________________________________________
#
#	CopyRight:	dGB Beheer B.V.
# 	Jan 2012	K. Tingdahl
#	RCS :		$Id$
#_______________________________________________________________________________

#Configure odversion.h
configure_file ( ${CMAKE_SOURCE_DIR}/include/Basic/odversion.h.in
		 ${CMAKE_BINARY_DIR}/include/Basic/odversion.h )

configure_file (${CMAKE_SOURCE_DIR}/CMakeModules/templates/.arcconfig.in
		${CMAKE_SOURCE_DIR}/.arcconfig @ONLY )

 if ( NOT (CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR ) )
    if ( UNIX )
	if ( APPLE )
	    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
			    ${CMAKE_SOURCE_DIR}/relinfo
			    ${CMAKE_BINARY_DIR}/Contents/Resources/relinfo )
	else()
	    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
                            ${CMAKE_SOURCE_DIR}/relinfo
			    ${CMAKE_BINARY_DIR}/relinfo )
	endif()
    else()
	    execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory
                            ${CMAKE_SOURCE_DIR}/relinfo
			    ${CMAKE_BINARY_DIR}/relinfo )
    endif()

    #Copy data as we generate stuff into the data directory, and that cannot
    #be in the source-dir
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory
		    ${CMAKE_SOURCE_DIR}/data
		    ${CMAKE_BINARY_DIR}/${OD_DATA_INSTALL_RELPATH} )
elseif( APPLE )
    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory
		    ${MISC_INSTALL_PREFIX}
		    WORKING_DIRECTORY ${CMAKE_BINARY_DIR} )
    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
		    ${CMAKE_SOURCE_DIR}/data
		    ${MISC_INSTALL_PREFIX}/data
		    WORKING_DIRECTORY ${CMAKE_BINARY_DIR} )
    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
		    ${CMAKE_SOURCE_DIR}/relinfo
		    ${MISC_INSTALL_PREFIX}/relinfo
		    WORKING_DIRECTORY ${CMAKE_BINARY_DIR} )
endif()



file(GLOB CMAKE_FILES CMakeModules/*.cmake )
file(GLOB TEMPLATE_FILES CMakeModules/templates/*.in )
set( CMAKE_FILES ${CMAKE_FILES} ${TEMPLATE_FILES} )
OD_ADD_SOURCE_FILES( ${CMAKE_FILES} )

#Install cmake things.
install ( FILES ${CMAKE_BINARY_DIR}/CMakeModules/FindOpendTect.cmake
	  DESTINATION ${MISC_INSTALL_PREFIX}/CMakeModules )
install ( DIRECTORY CMakeModules DESTINATION ${MISC_INSTALL_PREFIX}
	  PATTERN ".svn" EXCLUDE
	  PATTERN "*.swp" EXCLUDE
	  PATTERN "*.cmake~" EXCLUDE
	  PATTERN "sourcefiles*.txt*" EXCLUDE)

#install breakpad
install( PROGRAMS ${BREAKPAD_STACKWALK_EXECUTABLE}
             DESTINATION ${OD_LIB_INSTALL_PATH_RELEASE}
             CONFIGURATIONS Release )
#Adding breakpad related libs/exe to OD_THIRD_PARTY_LIBS
if ( OD_ENABLE_BREAKPAD )
    get_filename_component( STACKWALK_EXECUTABLE ${BREAKPAD_STACKWALK_EXECUTABLE} NAME )
    list( APPEND OD_THIRD_PARTY_LIBS ${STACKWALK_EXECUTABLE} )
    if ( WIN32 )
	list( APPEND OD_THIRD_PARTY_LIBS ${OD_BREAKPADBINS} )
    endif()
endif()

#install doc stuff
file( GLOB TUTHFILES plugins/Tut/*.h )
file( GLOB TUTCCFILES plugins/Tut/*.cc )
set( TUTFILES ${TUTHFILES} ${TUTCCFILES} plugins/Tut/CMakeLists.txt )
install( FILES ${TUTFILES} DESTINATION ${MISC_INSTALL_PREFIX}/doc/Programmer/pluginexample/plugins/Tut )
install( FILES doc/Programmer/style.css DESTINATION ${MISC_INSTALL_PREFIX}/doc/Programmer )
install( DIRECTORY dtect
	 DESTINATION ${MISC_INSTALL_PREFIX}
	 PATTERN ".svn" EXCLUDE )

file( GLOB UITUTHFILES plugins/uiTut/*.h )
file( GLOB UITUTCCFILES plugins/uiTut/*.cc )
set( UITUTFILES ${UITUTHFILES} ${UITUTCCFILES} plugins/uiTut/CMakeLists.txt )
install( FILES ${UITUTFILES} DESTINATION ${MISC_INSTALL_PREFIX}/doc/Programmer/pluginexample/plugins/uiTut )
install( FILES doc/Programmer/pluginexample/CMakeLists.txt
	 DESTINATION ${MISC_INSTALL_PREFIX}/doc/Programmer/pluginexample )

install( DIRECTORY doc/Programmer/batchprogexample
	 DESTINATION ${MISC_INSTALL_PREFIX}/doc/Programmer
	 PATTERN ".svn" EXCLUDE )

install( DIRECTORY doc/Credits/base
	 DESTINATION ${MISC_INSTALL_PREFIX}/doc/Credits
	 PATTERN ".svn" EXCLUDE )

OD_CURRENT_MONTH( MONTH )
OD_CURRENT_YEAR( YEAR )

configure_file( ${CMAKE_SOURCE_DIR}/CMakeModules/templates/license.txt.in
		${CMAKE_BINARY_DIR}/CMakeModules/license.txt @ONLY )

file( GLOB FLEXNETFILES doc/*.html )
foreach( FLEXNETFILE ${FLEXNETFILES} )
    install( FILES ${FLEXNETFILE} DESTINATION ${MISC_INSTALL_PREFIX}/doc )
endforeach()

install( DIRECTORY doc/Scripts
	 DESTINATION ${MISC_INSTALL_PREFIX}/doc
	 PATTERN ".svn" EXCLUDE )

#Install data
if ( APPLE )
    install ( DIRECTORY "data" DESTINATION ${MISC_INSTALL_PREFIX}/
	  PATTERN "install_files" EXCLUDE
	  PATTERN "icons.Classic" EXCLUDE
	  PATTERN ".svn" EXCLUDE )
else()
    install ( DIRECTORY "data" DESTINATION .
	  PATTERN "install_files" EXCLUDE
	  PATTERN "icons.Classic" EXCLUDE
	  PATTERN ".svn" EXCLUDE )
endif()

file( GLOB RELINFOFILES ${CMAKE_SOURCE_DIR}/relinfo/*.txt )
if ( APPLE )
    install ( FILES ${RELINFOFILES} DESTINATION Contents/Resources/relinfo )
else()
    install ( FILES ${RELINFOFILES} DESTINATION ./relinfo )
endif()

install( FILES CMakeLists.txt DESTINATION ${MISC_INSTALL_PREFIX} )

if( WIN32 )
    install( DIRECTORY bin/win32/rsm
	     DESTINATION ${MISC_INSTALL_PREFIX}
	     PATTERN ".svn" EXCLUDE )
endif()

file( GLOB TEXTFILES ${CMAKE_SOURCE_DIR}/data/install_files/unixscripts/*.txt )
if( UNIX OR APPLE )
    file( GLOB PROGRAMS ${CMAKE_SOURCE_DIR}/data/install_files/unixscripts/* )
    list( REMOVE_ITEM PROGRAMS
	  ${CMAKE_SOURCE_DIR}/data/install_files/unixscripts/.svn )
    list( REMOVE_ITEM PROGRAMS
	  ${CMAKE_SOURCE_DIR}/data/install_files/unixscripts/makeself )
    foreach( TEXTFILE ${TEXTFILES} )
        list( REMOVE_ITEM PROGRAMS ${TEXTFILE} )
    endforeach()

    install ( PROGRAMS ${PROGRAMS} DESTINATION ${MISC_INSTALL_PREFIX} )
endif()
install ( FILES ${TEXTFILES} DESTINATION ${MISC_INSTALL_PREFIX} )

if( APPLE )
    install( DIRECTORY data/install_files/macscripts/Contents
	     DESTINATION .
	     PATTERN ".svn" EXCLUDE )
    OD_CURRENT_YEAR( YEAR )
    set( BUNDLEEXEC od_main )
    set( BUNDLEICON "od.icns" )
    set( BUNDLEID "com.dgbes.opendtect" )
    set( BUNDLESTRING "OpendTect" )
    set( BUNDLENAME "${BUNDLESTRING}" )
    set ( INFOFILE CMakeModules/Info.plist )
    configure_file( ${CMAKE_SOURCE_DIR}/CMakeModules/templates/Info.plist.in
		    ${CMAKE_BINARY_DIR}/${INFOFILE} @ONLY )
    install( FILES ${CMAKE_BINARY_DIR}/${INFOFILE} DESTINATION "Contents" )
endif( APPLE )

if ( QT_QJPEG_PLUGIN_RELEASE )
    set( QJPEG ${QT_QJPEG_PLUGIN_RELEASE} )
endif()

set( LMHOSTID lmhostid )
if( WIN32 )
    if ( ${CMAKE_BUILD_TYPE} STREQUAL "Debug" )
	set( QJPEG ${QTDIR}/plugins/imageformats/qjpegd4.dll )
    elseif ( ${CMAKE_BUILD_TYPE} STREQUAL "Release" )
	set( QJPEG ${QTDIR}/plugins/imageformats/qjpeg4.dll )
    endif()
    if( ${OD_PLFSUBDIR} STREQUAL "win32" )
	set( MSVCPATH "C:/Program\ Files \(x86\)/Microsoft\ Visual\ Studio\ 12.0/VC/redist/x86/Microsoft.VC120.CRT" )
    elseif( ${OD_PLFSUBDIR} STREQUAL "win64" )
	set( MSVCPATH "C:/Program\ Files \(x86\)/Microsoft\ Visual\ Studio\ 12.0/VC/redist/x64/Microsoft.VC120.CRT" )
    endif()
    install( DIRECTORY ${CMAKE_BINARY_DIR}/${OD_LIB_RELPATH_DEBUG}
	    DESTINATION bin/${OD_PLFSUBDIR}
	    CONFIGURATIONS Debug
	    FILES_MATCHING
	    PATTERN *.pdb
	)
    install( DIRECTORY ${CMAKE_BINARY_DIR}/${OD_LIB_RELPATH_DEBUG}
	    DESTINATION bin/${OD_PLFSUBDIR}
	    CONFIGURATIONS Debug
	    FILES_MATCHING
	    PATTERN *.lib
	)
    install( DIRECTORY ${CMAKE_BINARY_DIR}/${OD_LIB_RELPATH_RELEASE}
	    DESTINATION bin/${OD_PLFSUBDIR}
	    CONFIGURATIONS Release
	    FILES_MATCHING
	    PATTERN *.lib
	)
    set( LMHOSTID "lmhostid.exe" )
endif()

install( PROGRAMS ${QJPEG} DESTINATION ${MISC_INSTALL_PREFIX}/imageformats )
if ( WIN32 )
    install( PROGRAMS ${CMAKE_SOURCE_DIR}/bin/${OD_PLFSUBDIR}/${LMHOSTID}
	     DESTINATION ${OD_EXEC_INSTALL_PATH_RELEASE}
	     CONFIGURATIONS Release )
    install( PROGRAMS ${CMAKE_SOURCE_DIR}/bin/${OD_PLFSUBDIR}/${LMHOSTID}
	     DESTINATION ${OD_EXEC_INSTALL_PATH_DEBUG}
	     CONFIGURATIONS Debug )
    install( PROGRAMS ${CMAKE_SOURCE_DIR}/bin/od_main_debug.bat
	     DESTINATION ${OD_EXEC_INSTALL_PATH_RELEASE}
	     CONFIGURATIONS Release )
else()
    install( PROGRAMS ${CMAKE_SOURCE_DIR}/bin/${OD_PLFSUBDIR}/${LMHOSTID}
	     DESTINATION ${OD_EXEC_OUTPUT_RELPATH} )
endif()

if ( NOT EXISTS ${CMAKE_BINARY_DIR}/${OD_EXEC_RELPATH_DEBUG} )
    file ( MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${OD_EXEC_RELPATH_DEBUG} )
endif()
if ( NOT EXISTS ${CMAKE_BINARY_DIR}/${OD_EXEC_RELPATH_RELEASE} )
    file ( MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${OD_EXEC_RELPATH_RELEASE} )
endif()

execute_process( COMMAND ${CMAKE_COMMAND} -E copy_if_different
	       ${CMAKE_SOURCE_DIR}/bin/${OD_PLFSUBDIR}/${LMHOSTID}
	       ${CMAKE_BINARY_DIR}/${OD_EXEC_RELPATH_RELEASE}/ )
execute_process( COMMAND ${CMAKE_COMMAND} -E copy_if_different
	       ${CMAKE_SOURCE_DIR}/bin/${OD_PLFSUBDIR}/${LMHOSTID}
	       ${CMAKE_BINARY_DIR}/${OD_EXEC_RELPATH_DEBUG}/ )

if( EXISTS ${MSVCPATH} )
    file( GLOB MSVCDLLS ${MSVCPATH}/*.dll )
    foreach( DLL ${MSVCDLLS} )
	install( FILES ${DLL} DESTINATION ${OD_EXEC_INSTALL_PATH_RELEASE} CONFIGURATIONS Release )
	get_filename_component( FILENAME ${DLL} NAME )
	list( APPEND OD_THIRD_PARTY_LIBS ${FILENAME} )
    endforeach()
endif()

FILE( GLOB SCRIPTS ${CMAKE_SOURCE_DIR}/bin/od_* )
if ( OD_ENABLE_BREAKPAD )
    FILE( GLOB BREAKPADSCRIPTS ${CMAKE_SOURCE_DIR}/bin/process_dumpfile* )
    set ( SCRIPTS ${SCRIPTS} ${BREAKPADSCRIPTS} )
endif()

install( PROGRAMS ${SCRIPTS} DESTINATION ${MISC_INSTALL_PREFIX}/bin )
install( PROGRAMS ${CMAKE_SOURCE_DIR}/bin/mksethdir DESTINATION ${MISC_INSTALL_PREFIX}/bin )
install( FILES ${CMAKE_SOURCE_DIR}/bin/macterm.in DESTINATION ${MISC_INSTALL_PREFIX}/bin )

OD_CURRENT_DATE( DATE )
configure_file( ${CMAKE_SOURCE_DIR}/CMakeModules/templates/buildinfo.h.in
		${CMAKE_BINARY_DIR}/include/Basic/buildinfo.h @ONLY )

#Installing source
install( DIRECTORY ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/include
		   ${CMAKE_SOURCE_DIR}/plugins DESTINATION ${MISC_INSTALL_PREFIX}/
	 FILE_PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
	 FILES_MATCHING PATTERN "*.h" PATTERN "*.c" PATTERN "*.cc" PATTERN "*.xpm"
			PATTERN "*.ico" PATTERN "*.rc" PATTERN "*.txt"
			PATTERN ".svn" EXCLUDE PATTERN CMakeFiles EXCLUDE )

#Installing cmake genetated files from CMAKE_BINARY_DIR directory
if ( NOT "${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}" )
    install( DIRECTORY ${CMAKE_BINARY_DIR}/src ${CMAKE_BINARY_DIR}/include
		       ${CMAKE_BINARY_DIR}/plugins DESTINATION ${MISC_INSTALL_PREFIX}/
	     FILE_PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
	     FILES_MATCHING PATTERN "*.h" PATTERN "*.c" PATTERN "*.cc" PATTERN "*.xpm"
			    PATTERN "*.ico" PATTERN "*.rc" PATTERN "*.txt"
			    PATTERN ".svn" EXCLUDE PATTERN CMakeFiles EXCLUDE )
endif()
