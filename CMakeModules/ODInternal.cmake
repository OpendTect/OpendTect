#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

#Configure odversion.h
configure_file ( ${CMAKE_SOURCE_DIR}/include/Basic/odversion.h.in
		 ${CMAKE_BINARY_DIR}/include/Basic/odversion.h )

configure_file (${CMAKE_SOURCE_DIR}/CMakeModules/templates/.arcconfig.in
		${CMAKE_SOURCE_DIR}/.arcconfig @ONLY )

set( LMUTIL lmutil )
if ( WIN32 )
    set( LMUTIL "${LMUTIL}.exe" )
endif()

if ( NOT ${BUILDINSRC} )
    if ( UNIX )
	if ( APPLE )
	    if ( NOT EXISTS "${CMAKE_BINARY_DIR}/Contents/Resources" )
		file( MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/Contents/Resources" )
	    endif()
	    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
			    ${CMAKE_SOURCE_DIR}/relinfo
			    ${CMAKE_BINARY_DIR}/Contents/Resources/relinfo )
	else()
	    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink
                            ${CMAKE_SOURCE_DIR}/relinfo
			    ${CMAKE_BINARY_DIR}/relinfo )
	endif()
    else()
	file( COPY "${CMAKE_SOURCE_DIR}/relinfo"
	      DESTINATION "${CMAKE_BINARY_DIR}" )
    endif()

    #Copy data as we generate stuff into the data directory, and that cannot
    #be in the source-dir
    file( COPY "${CMAKE_SOURCE_DIR}/data"
	  DESTINATION "${CMAKE_BINARY_DIR}/${MISC_INSTALL_PREFIX}" )
    file( COPY "${CMAKE_SOURCE_DIR}/bin/${OD_PLFSUBDIR}/${LMUTIL}"
	  DESTINATION "${CMAKE_BINARY_DIR}/bin/${OD_PLFSUBDIR}/lm.dgb" )
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


file( GLOB CMAKE_FILES CMakeModules/*.cmake )
file( GLOB TEMPLATE_FILES CMakeModules/templates/*.in )
set( CMAKE_FILES ${CMAKE_FILES} ${TEMPLATE_FILES} )
OD_ADD_SOURCE_FILES( ${CMAKE_FILES} )

#Install cmake things.
install ( FILES "${CMAKE_BINARY_DIR}/CMakeModules/FindOpendTect.cmake"
	  DESTINATION "${MISC_INSTALL_PREFIX}/CMakeModules" )
install ( DIRECTORY CMakeModules
	  DESTINATION "${MISC_INSTALL_PREFIX}"
	  PATTERN "*.swp" EXCLUDE
	  PATTERN "*.cmake~" EXCLUDE
	  PATTERN "sourcefiles*.txt*" EXCLUDE)

#install doc stuff
file( GLOB TUTHFILES plugins/Tut/*.h )
file( GLOB TUTCCFILES plugins/Tut/*.cc )
set( TUTFILES ${TUTHFILES} ${TUTCCFILES} plugins/Tut/CMakeLists.txt )
install( FILES ${TUTFILES}
	 DESTINATION "${MISC_INSTALL_PREFIX}/doc/Programmer/pluginexample/plugins/Tut" )
install( FILES doc/Videos.od
	 DESTINATION "${MISC_INSTALL_PREFIX}/doc" )
install( DIRECTORY dtect
	 DESTINATION "${MISC_INSTALL_PREFIX}" )

file( GLOB UITUTHFILES plugins/uiTut/*.h )
file( GLOB UITUTCCFILES plugins/uiTut/*.cc )
set( UITUTFILES ${UITUTHFILES} ${UITUTCCFILES} plugins/uiTut/CMakeLists.txt )
install( FILES ${UITUTFILES}
	 DESTINATION "${MISC_INSTALL_PREFIX}/doc/Programmer/pluginexample/plugins/uiTut" )
install( FILES doc/Programmer/pluginexample/CMakeLists.txt
	 DESTINATION "${MISC_INSTALL_PREFIX}/doc/Programmer/pluginexample" )

install( DIRECTORY doc/Programmer/batchprogexample
	 DESTINATION "${MISC_INSTALL_PREFIX}/doc/Programmer" )

string(TIMESTAMP YEAR %Y)
configure_file( ${CMAKE_SOURCE_DIR}/CMakeModules/templates/license.txt.in
		${CMAKE_BINARY_DIR}/CMakeModules/license.txt @ONLY )

file( GLOB FLEXNETFILES doc/*.html )
foreach( FLEXNETFILE ${FLEXNETFILES} )
    install( FILES ${FLEXNETFILE}
	     DESTINATION "${MISC_INSTALL_PREFIX}/doc" )
endforeach()

install( DIRECTORY doc/Scripts
	 DESTINATION "${MISC_INSTALL_PREFIX}/doc" )

#Install data
install ( DIRECTORY "${CMAKE_BINARY_DIR}/${OD_DATA_INSTALL_RELPATH}"
      DESTINATION "${MISC_INSTALL_PREFIX}"
      USE_SOURCE_PERMISSIONS
      PATTERN "install_files" EXCLUDE
      PATTERN "icons.Classic" EXCLUDE
      PATTERN ".gitignore" EXCLUDE )

file( GLOB RELINFOFILES "${CMAKE_SOURCE_DIR}/relinfo/*.txt" )
install ( FILES ${RELINFOFILES}
	  DESTINATION "${MISC_INSTALL_PREFIX}/relinfo" )

#Install python module
if ( EXISTS "${CMAKE_SOURCE_DIR}/external/odpy" )
    install ( DIRECTORY "${CMAKE_SOURCE_DIR}/external/odpy/odpy"
	      DESTINATION "${MISC_INSTALL_PREFIX}/bin/python"
	      PATTERN ".swp" EXCLUDE PATTERN "__pycache__" EXCLUDE )
endif()
if ( EXISTS "${CMAKE_SOURCE_DIR}/external/safety" )
    install ( DIRECTORY "${CMAKE_SOURCE_DIR}/external/safety/safety"
	      DESTINATION "${MISC_INSTALL_PREFIX}/bin/python"
	      PATTERN ".*.swp" EXCLUDE PATTERN "__pycache__" EXCLUDE )
endif()
if ( EXISTS "${CMAKE_SOURCE_DIR}/external/marshmallow" )
    install ( DIRECTORY "${CMAKE_SOURCE_DIR}/external/marshmallow/src/marshmallow"
	      DESTINATION "${MISC_INSTALL_PREFIX}/bin/python"
	      PATTERN ".*.swp" EXCLUDE PATTERN "__pycache__" EXCLUDE )
endif()

install( FILES CMakeLists.txt
	 DESTINATION "${MISC_INSTALL_PREFIX}" )

file( GLOB TEXTFILES ${CMAKE_SOURCE_DIR}/data/install_files/unixscripts/*.txt )
if( UNIX OR APPLE )
    file( GLOB PROGRAMS ${CMAKE_SOURCE_DIR}/data/install_files/unixscripts/* )
    list( REMOVE_ITEM PROGRAMS
	  ${CMAKE_SOURCE_DIR}/data/install_files/unixscripts/makeself )
    foreach( TEXTFILE ${TEXTFILES} )
        list( REMOVE_ITEM PROGRAMS ${TEXTFILE} )
    endforeach()

    install( PROGRAMS ${PROGRAMS}
	     DESTINATION "${MISC_INSTALL_PREFIX}" )
endif()
install( FILES ${TEXTFILES}
	 DESTINATION "${MISC_INSTALL_PREFIX}" )

if( APPLE )
    install( DIRECTORY data/install_files/macscripts/Contents
	     DESTINATION . )
    set( BUNDLEEXEC "od_main" )
    set( BUNDLEICON "od.icns" )
    set( BUNDLEID "com.dgbes.opendtect" )
    set( BUNDLESTRING "OpendTect" )
    set( BUNDLENAME "${BUNDLESTRING}" )
    set( BUNDLEVERSION "${OpendTect_VERSION_MAJOR}.${OpendTect_VERSION_MINOR}.${OpendTect_VERSION_PATCH}" )
    set( INFOFILE CMakeModules/Info.plist )
    configure_file( ${CMAKE_SOURCE_DIR}/CMakeModules/templates/Info.plist.in
		    ${CMAKE_BINARY_DIR}/${INFOFILE} @ONLY )
    install( FILES ${CMAKE_BINARY_DIR}/${INFOFILE}
	     DESTINATION "Contents" )
endif( APPLE )

if( WIN32 )
    set( CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION "${OD_RUNTIME_DIRECTORY}" )
    include( InstallRequiredSystemLibraries )
elseif ( NOT APPLE )
    get_filename_component( CXXPATH ${CMAKE_CXX_COMPILER} DIRECTORY )
    get_filename_component( CXXPATH ${CXXPATH} DIRECTORY )
    set( LIBSEARCHPATHS "${CXXPATH}/lib64" )
    od_find_library( LIBSTDLOC libstdc++.so.6 )
    if ( LIBSTDLOC AND EXISTS "${LIBSTDLOC}" )
	OD_INSTALL_LIBRARY( "${LIBSTDLOC}" "${OD_LIBRARY_DIRECTORY}" )
    else()
	message( SEND_ERROR "Required system library not found: libstdc++" )
    endif()
    od_find_library( LIBGCCLOC libgcc_s.so.1 )
    if ( LIBGCCLOC AND EXISTS "${LIBSTDLOC}" )
	OD_INSTALL_LIBRARY( "${LIBGCCLOC}" "${OD_LIBRARY_DIRECTORY}" )
    else()
	message( SEND_ERROR "Required system library not found: libgcc_s" )
    endif()
endif()

if ( WIN32 )
    install( FILES ${CMAKE_SOURCE_DIR}/bin/od_main_debug.bat
	     DESTINATION "${OD_RUNTIME_DIRECTORY}"
	     CONFIGURATIONS MinSizeRel;RelWithDebInfo;Release )
endif()
install( PROGRAMS "${CMAKE_SOURCE_DIR}/bin/${OD_PLFSUBDIR}/${LMUTIL}"
	 DESTINATION "${MISC_INSTALL_PREFIX}/bin/${OD_PLFSUBDIR}/lm.dgb" )

FILE( GLOB SCRIPTS ${CMAKE_SOURCE_DIR}/bin/od_* )
if ( OD_ENABLE_BREAKPAD )
    FILE( GLOB BREAKPADSCRIPTS ${CMAKE_SOURCE_DIR}/bin/process_dumpfile* )
    set ( SCRIPTS ${SCRIPTS} ${BREAKPADSCRIPTS} )
endif()

install( PROGRAMS ${SCRIPTS} DESTINATION ${MISC_INSTALL_PREFIX}/bin )
install( PROGRAMS ${CMAKE_SOURCE_DIR}/bin/mksethdir DESTINATION ${MISC_INSTALL_PREFIX}/bin )
if( UNIX )
    install( PROGRAMS ${CMAKE_SOURCE_DIR}/bin/init_dtect_GL DESTINATION ${MISC_INSTALL_PREFIX}/bin )
endif()
install( FILES ${CMAKE_SOURCE_DIR}/bin/macterm.in DESTINATION ${MISC_INSTALL_PREFIX}/bin )

string(TIMESTAMP DATE "%a %d %b %Y %H:%M:%S UTC" UTC )
configure_file( ${CMAKE_SOURCE_DIR}/CMakeModules/templates/buildinfo.h.in
		${CMAKE_BINARY_DIR}/include/Basic/buildinfo.h @ONLY )

#Installing source
install( DIRECTORY "${CMAKE_SOURCE_DIR}/src"
		   "${CMAKE_SOURCE_DIR}/include"
		   "${CMAKE_SOURCE_DIR}/plugins"
		   "${CMAKE_SOURCE_DIR}/spec"
	 DESTINATION "${MISC_INSTALL_PREFIX}"
	 CONFIGURATIONS Debug
	 FILE_PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
	 FILES_MATCHING PATTERN "*.h" PATTERN "*.c" PATTERN "*.cc" PATTERN "*.xpm"
			PATTERN "*.ico" PATTERN "*.rc" PATTERN "*.txt"
			EXCLUDE PATTERN CMakeFiles EXCLUDE )

#Installing cmake generated files from CMAKE_BINARY_DIR directory
if ( NOT ${BUILDINSRC} )
    install( DIRECTORY "${CMAKE_BINARY_DIR}/src"
		       "${CMAKE_BINARY_DIR}/include"
		       "${CMAKE_BINARY_DIR}/plugins"
		       "${CMAKE_BINARY_DIR}/spec"
	     DESTINATION "${MISC_INSTALL_PREFIX}"
	     CONFIGURATIONS Debug
	     FILE_PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
	     FILES_MATCHING PATTERN "*.h" PATTERN "*.c" PATTERN "*.cc" PATTERN "*.xpm"
			    PATTERN "*.ico" PATTERN "*.rc" PATTERN "*.txt"
			    EXCLUDE PATTERN CMakeFiles EXCLUDE )
endif()
