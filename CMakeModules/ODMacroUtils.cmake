#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

# OD_INIT_MODULE - Macro that setups a number of variables for compiling
#		   OpendTect.
#
# Input variables:
# 
# OD_MODULE_NAME			: Name of the module, or the plugin
# OD_SUBSYSTEM                          : "od" or "dgb"
# OD_MODULE_DEPS			: List of other modules that this
#					  module is dependent on.
# OD_MODULE_SOURCES			: Sources that should go into the library
# OD_USEBATCH				: Whether to include include/Batch 
# OD_USEQT				: Dependency on Qt is enabled if set.
# OD_INSTQT				: Runtime libraries from Qt are required.
#					  value should be a valid Qt component like
#					  Core, Sql, Network, Gui
# OD_USEOSG				: Dependency on OSG is enabled if set.
# OD_USEZLIB				: Dependency on ZLib is enabled if set.
# OD_LINKOPENSSL			: Dependency on OpenSSL::SSL is enabled if set.
# OD_LINKCRYPTO				: Dependency on OpenSSL::Crypto is enabled if set.
# OD_LINKPROJ				: Dependency on proj is enabled if set
# OD_LINKSQLITE				: Dependency on sqlite is enabled if set
# OD_USEHDF5				: Dependency on HDF5 is enabled if set.
# OD_USEBREAKPAD			: Runtime availability on breakpad is enabled if set
# OD_USEOPENSSL				: Runtime availability on OpenSSL::SSL is enabled if set.
# OD_USECRYPTO				: Runtime availability on OpenSSL::Crypto is enabled if set.
# OD_USEPROJ				: Runtime availability on proj is enabled if set
# OD_USESQLITE				: Runtime availability on sqlite is enabled if set
# OD_IS_PLUGIN				: Tells if this is a plugin (if set)
# OD_PLUGINMODULES			: A list of eventual sub-modules of
#					  a plugin. Each submodule must have an
#					  plugins/{OD_MODULE_NAME}/src/${PLUGINMODULE}/CMakeFile.txt
# OD_MODULE_INCLUDESYSPATH		: The system includepath needed to compile the
#					  source- files of the module.
# OD_MODULE_EXTERNAL_LIBS		: External libraries needed for the module
#					  to link against.
# OD_MODULE_EXTERNAL_SYSLIBS		: System libraries needed for the module to link against.
#					  Will not be installed with the project.
# OD_MODULE_EXTERNAL_RUNTIME_LIBS	: External libraries needed for the module
#					  at runtime (the module will not depend on them)
# OD_MODULE_COMPILE_DEFINITIONS		: A semicolon-separated list of compile definitions for this module
# OD_MODULE_COMPILE_OPTIONS		: A semicolon-separated list of compile options for this module
# OD_MODULE_LINK_OPTIONS		: A semicolon-separated list of link options for this module
#####################################
#
# Output variables:
#
# OD_${OD_MODULE_NAME}_INCLUDEPATH 	: The path(s) to the headerfiles of the
#					  module. Normally one single one, but
#					  may have multiple paths in plugins.
# OD_${OD_MODULE_NAME}_DEPS		: The modules this module is dependent
#					  on.
# OD_${OD_MODULE_NAME}_RUNTIMEPATH	: The runtime path for its own library,
#					  and all external libraries it is
#					  dependent on.
# OD_${OD_MODULE_NAME}_PROGS		: The list of all executable that 
#					  belong to a given module.
# OD_MODULE_NAMES_${OD_SUBSYSTEM}	: A list of all modules
# OD_CORE_MODULE_NAMES_${OD_SUBSYSTEM}  : A list of all non-plugin modules
#####################################
#
# Internal variables
#
# OD_MODULE_INCLUDEPATH		: The includepath needed to compile the source-
#				  files of the module.
# OD_MODULE_RUNTIMEPATH		: All directories that are needed at runtime
# OD_MODULE_INTERNAL_LIBS	: All OD libraries needed for the module
# OD_MODULE_ALL_EXTERNAL_LIBS	: All external libraries needed for the module

list( APPEND SETUPNMS
       USEQT
       INSTQT
       USEOSG
       USEZLIB
       LINKOPENSSL
       LINKCRYPTO
       LINKPROJ
       LINKSQLITE
       USEHDF5
       USEBREAKPAD
       USEOPENSSL
       USECRYPTO
       USEPROJ
       USESQLITE
)


macro( OD_SET_UNICODE MODNM )
    if ( WIN32 AND QT_VERSION_MAJOR GREATER_EQUAL 6 )
	qt6_disable_unicode_defines( ${MODNM} )
    endif()
endmacro( OD_SET_UNICODE )


macro( OD_GET_MODULE_DEPS )
    OD_GET_ALL_DEPS( ${OD_MODULE_NAME} OD_MODULE_INTERNAL_LIBS )
    foreach ( DEP ${OD_MODULE_INTERNAL_LIBS} )
	#Add dependencies to include-path
	list(APPEND OD_MODULE_INCLUDEPATH ${OD_${DEP}_INCLUDEPATH} )
	list(APPEND OD_MODULE_RUNTIMEPATH ${OD_${DEP}_RUNTIMEPATH} )
	if ( NOT OD_SUBSYSTEM MATCHES ${OD_CORE_SUBSYSTEM} AND 
	     "${OD_DISABLE_EXTERNAL_LIBS_CHECK}" STREQUAL "OFF" )
	    set( NEEDSSETUP FALSE )
	    foreach( USENM ${SETUPNMS} )
		if ( OD_${DEP}_${USENM} )
		    list( APPEND OD_${USENM} ${OD_${DEP}_${USENM}} )
	            set( NEEDSSETUP TRUE )
		    unset( OD_${DEP}_${USENM} PARENT_SCOPE )
		endif()
	    endforeach()
	    if ( NEEDSETUP )
	        list(APPEND OD_MODULE_EXTERNAL_LIBS ${OD_${DEP}_EXTERNAL_LIBS} )
	        list(APPEND OD_MODULE_EXTERNAL_RUNTIME_LIBS ${OD_${DEP}_EXTERNAL_RUNTIME_LIBS} )
	    endif()
	endif()
    endforeach()
endmacro( OD_GET_MODULE_DEPS )

macro( OD_INIT_MODULE )

get_filename_component( OD_MODULE_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME )

set ( OD_MODULE_HAS_LIBRARY ${OD_MODULE_SOURCES} )

#test for keywords not allowed in source

#Add this module to the list
if ( OD_MODULE_HAS_LIBRARY )
    set( OD_MODULE_NAMES_${OD_SUBSYSTEM} ${OD_MODULE_NAMES_${OD_SUBSYSTEM}}
					 ${OD_MODULE_NAME} PARENT_SCOPE )

    #Create init-header 
    OD_CREATE_INIT_HEADER()
endif(  OD_MODULE_HAS_LIBRARY )

#Add all module dependencies
set( OD_${OD_MODULE_NAME}_DEPS ${OD_MODULE_DEPS} ${OD_EXT_MODULE_DEPS} )

#Setup all deps and set runtime and includepath
if( OD_MODULE_DEPS )
    OD_GET_MODULE_DEPS()
    foreach( USENM ${SETUPNMS} )
	if ( OD_${USENM} )
	    list( REMOVE_DUPLICATES OD_${USENM} )
	endif()
    endforeach()
endif()

if( OD_USEOSG )
    OD_SETUP_OSG()
endif()

#Add Qt-stuff
if( OD_USEQT OR OD_INSTQT )
   OD_SETUP_QT()
endif()

#Must be after QT
if( (UNIX OR WIN32) AND OD_USEZLIB )
    OD_SETUP_ZLIB()
endif()

if ( OD_USEBREAKPAD )
    OD_SETUP_BREAKPAD()
endif()

if( OD_USEOPENSSL OR OD_LINKOPENSSL )
    OD_SETUP_OPENSSL()
endif()

if ( OD_USESQLITE OR OD_LINKSQLITE )
    OD_SETUP_SQLITE()
endif()

if( OD_USECRYPTO OR OD_LINKCRYPTO )
    OD_SETUP_CRYPTO()
endif()

if( OD_USEPROJ OR OD_LINKPROJ )
    OD_SETUP_PROJ()
endif()

if( OD_USEHDF5 )
    OD_SETUP_HDF5()
endif()

OD_SETUP_DEBUGFLAGS()

if( "${PROJECT_OUTPUT_DIR}" STREQUAL "" )
    set( PROJECT_OUTPUT_DIR "${CMAKE_BINARY_DIR}" )
endif()

#Add current module to include-path
if ( OD_MODULE_HAS_LIBRARY )
    if ( OD_IS_PLUGIN )

        set( PLUGINDIR ${CMAKE_CURRENT_SOURCE_DIR} )
	list( APPEND OD_${OD_MODULE_NAME}_INCLUDEPATH ${PLUGINDIR} ${INITHEADER_DIR} )
	foreach ( OD_PLUGINSUBDIR ${OD_PLUGINMODULES} )
	    list(APPEND OD_${OD_MODULE_NAME}_INCLUDEPATH
		${PLUGINDIR}/include/${OD_PLUGINSUBDIR})
	    include(${PLUGINDIR}/src/${OD_PLUGINSUBDIR}/CMakeLists.txt)
	endforeach()

	# Record alo-entries
	if ( NOT DEFINED OD_NO_ALO_ENTRY )
	    set( OD_ALO_NAME ${OD_MODULE_NAME} )
	    OD_ADD_ALO_ENTRIES( ${OD_PLUGIN_ALO_EXEC} )
	endif()
    else()

	set( OD_CORE_MODULE_NAMES_${OD_SUBSYSTEM}
	     ${OD_CORE_MODULE_NAMES_${OD_SUBSYSTEM}}
		${OD_MODULE_NAME} PARENT_SCOPE )
	list( APPEND OD_${OD_MODULE_NAME}_INCLUDEPATH
	    ${INITHEADER_DIR} )

	if ( EXISTS ${CMAKE_SOURCE_DIR}/include/${OD_MODULE_NAME} )
	    list( APPEND OD_${OD_MODULE_NAME}_INCLUDEPATH 
		${CMAKE_SOURCE_DIR}/include/${OD_MODULE_NAME} )
	endif()

	if ( EXISTS ${CMAKE_SOURCE_DIR}/spec/${OD_MODULE_NAME} )
	    list( APPEND OD_${OD_MODULE_NAME}_INCLUDEPATH 
		${CMAKE_SOURCE_DIR}/spec/${OD_MODULE_NAME} )
	endif()
    endif(OD_IS_PLUGIN)

    list( REMOVE_DUPLICATES OD_${OD_MODULE_NAME}_INCLUDEPATH )

    #Add all headerfiles to be included in the library (nice in IDEs)
    foreach ( INCDIR ${OD_${OD_MODULE_NAME}_INCLUDEPATH} )
	file ( GLOB INCFILES ${INCDIR}/*.h )
	file ( GLOB XPMFILES ${INCDIR}/*.xpm )
	file ( GLOB INFILES ${INCDIR}/*.in )
	set( INCFILES ${INCFILES} ${XPMFILES} ${INFILES} )
	if ( INCFILES )
	    list ( APPEND OD_MODULE_INCFILES ${INCFILES} )
	endif( INCFILES )
    endforeach()

    if (NOT OD_IS_PLUGIN)
	file ( GLOB SRCHEADERFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.h )
	file ( GLOB SRCXPMFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.xpm )
	file ( GLOB SRCINFILES ${CMAKE_CURRENT_SOURCE_DIR}/*.in )
	set( SRCFILES ${SRCHEADERFILES} ${SRCXPMFILES} ${SRCINFILES} )
	if ( SRCFILES )
	    list ( APPEND OD_MODULE_INCFILES ${SRCFILES} )
	endif()
	list ( APPEND SRCHEADERFILES ${SRCXPMFILES} )
    endif()

endif ( OD_MODULE_HAS_LIBRARY )

list( APPEND OD_MODULE_INCLUDEPATH ${OD_${OD_MODULE_NAME}_INCLUDEPATH} )

list( APPEND OD_MODULE_ALL_EXTERNAL_LIBS ${OD_MODULE_EXTERNAL_LIBS} )
list( APPEND OD_MODULE_ALL_EXTERNAL_LIBS ${OD_MODULE_EXTERNAL_RUNTIME_LIBS} )

guess_runtime_library_dirs( EXTERNAL_RUNTIMEPATH ${OD_MODULE_EXTERNAL_LIBS} )
guess_runtime_library_dirs( EXTERNAL_EXTRUNTIMEPATH ${OD_MODULE_EXTERNAL_RUNTIME_LIBS} )
list( APPEND EXTERNAL_RUNTIMEPATH ${EXTERNAL_EXTRUNTIMEPATH} )
if ( OD_MODULE_EXTERNAL_LIBS )
    set( OD_${OD_MODULE_NAME}_EXTERNAL_LIBS "${OD_MODULE_EXTERNAL_LIBS}" )
    if( OD_${OD_MODULE_NAME}_EXTERNAL_LIBS )
	list( REMOVE_DUPLICATES OD_${OD_MODULE_NAME}_EXTERNAL_LIBS )
    endif()
endif()
if ( OD_MODULE_EXTERNAL_RUNTIME_LIBS )
    set( OD_${OD_MODULE_NAME}_EXTERNAL_RUNTIME_LIBS "${OD_MODULE_EXTERNAL_RUNTIME_LIBS}" )
    if( OD_${OD_MODULE_NAME}_EXTERNAL_RUNTIME_LIBS )
	list( REMOVE_DUPLICATES OD_${OD_MODULE_NAME}_EXTERNAL_RUNTIME_LIBS )
    endif()
endif()
list( APPEND OD_${OD_MODULE_NAME}_RUNTIMEPATH ${EXTERNAL_RUNTIMEPATH} )
if ( OD_${OD_MODULE_NAME}_RUNTIMEPATH )
    list( REMOVE_DUPLICATES OD_${OD_MODULE_NAME}_RUNTIMEPATH )
endif()
list( APPEND OD_MODULE_RUNTIMEPATH ${OD_${OD_MODULE_NAME}_RUNTIMEPATH} )

#Clean up the lists
list( REMOVE_DUPLICATES OD_MODULE_INCLUDEPATH )
if ( OD_MODULE_RUNTIMEPATH )
    list( REMOVE_DUPLICATES OD_MODULE_RUNTIMEPATH )
endif()

if ( OD_MODULE_HAS_LIBRARY )
    #Export dependencies
    if ( ${OD_SUBSYSTEM} MATCHES ${OD_CORE_SUBSYSTEM} )
        OD_EXPORT_SETUP()
    endif()
    set( OD_${OD_MODULE_NAME}_DEPS ${OD_MODULE_DEPS} PARENT_SCOPE )
    set( OD_${OD_MODULE_NAME}_INCLUDEPATH
	 ${OD_${OD_MODULE_NAME}_INCLUDEPATH} PARENT_SCOPE )
    set( OD_${OD_MODULE_NAME}_EXTERNAL_LIBS
	 ${OD_${OD_MODULE_NAME}_EXTERNAL_LIBS} PARENT_SCOPE )
    set( OD_${OD_MODULE_NAME}_EXTERNAL_RUNTIME_LIBS
	 ${OD_${OD_MODULE_NAME}_EXTERNAL_RUNTIME_LIBS} PARENT_SCOPE )
    set( OD_${OD_MODULE_NAME}_RUNTIMEPATH
	 ${OD_${OD_MODULE_NAME}_RUNTIMEPATH} PARENT_SCOPE )
endif( OD_MODULE_HAS_LIBRARY )

#Extract static libraries
foreach( STATIC_LIB ${OD_MODULE_STATIC_LIBS} )
    if ( NOT EXISTS ${STATIC_LIB} )
	message( FATAL_ERROR "${STATIC_LIB} does not exist" )
    endif()

    get_filename_component( STATIC_LIB_NAME ${STATIC_LIB} NAME )
    set( STATIC_LIB_DIR
         ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${OD_MODULE_NAME}.dir/${STATIC_LIB_NAME}.dir )
    if ( WIN32 )
	set ( SHARED_LIB_COMMAND ${OpendTect_DIR}/dtect/extract_static_lib.cmd ${STATIC_LIB} ${OD_PLFSUBDIR} )
    else()
	set ( SHARED_LIB_COMMAND ${CMAKE_AR} x ${STATIC_LIB} )
    endif()
    if ( NOT EXISTS ${STATIC_LIB_DIR} )
	file ( MAKE_DIRECTORY ${STATIC_LIB_DIR} )
    endif()

    execute_process( 
	COMMAND ${SHARED_LIB_COMMAND}
	WORKING_DIRECTORY ${STATIC_LIB_DIR} )

    if ( WIN32 )
	file ( GLOB STATIC_LIB_FILES ${STATIC_LIB_DIR}/*.obj )
    else()
	file( GLOB STATIC_LIB_FILES ${STATIC_LIB_DIR}/*${CMAKE_C_OUTPUT_EXTENSION} )
    endif()

    list( APPEND OD_STATIC_OUTFILES ${STATIC_LIB_FILES} )

    add_custom_command( OUTPUT ${STATIC_LIB_FILES}
	        COMMAND ${SHARED_LIB_COMMAND}
                WORKING_DIRECTORY ${STATIC_LIB_DIR}
		DEPENDS ${STATIC_LIB}
                COMMENT "Extracting shared library ${STATIC_LIB_NAME}" )

    foreach( STATIC_LIB_FILE ${STATIC_LIB_FILES} )
	set_property( DIRECTORY APPEND
		PROPERTY ADDITIONAL_MAKE_CLEAN_FILES ${STATIC_LIB_FILE} )
    endforeach()

endforeach()

#Setup library & its deps
if ( OD_MODULE_HAS_LIBRARY )
    set( OD_LIB_DEP_LIBS ${EXTRA_LIBS} ${OD_MODULE_DEPS} )
    if ( NOT OD_SUBSYSTEM MATCHES ${OD_CORE_SUBSYSTEM} )
	list( APPEND OD_LIB_DEP_LIBS ${OD_MODULE_INTERNAL_LIBS} )
	list( REMOVE_DUPLICATES OD_LIB_DEP_LIBS )
    endif()
else()
    set( OD_EXEC_DEP_LIBS ${EXTRA_LIBS} ${OD_MODULE_INTERNAL_LIBS} )
endif()

if ( OD_MODULE_HAS_LIBRARY )
    add_library( ${OD_MODULE_NAME} SHARED ${OD_MODULE_SOURCES}
		 ${QT_MOC_OUTFILES}
		 ${OD_MODULE_INCFILES}
		 ${OD_STATIC_OUTFILES} )
    target_link_libraries(
	    ${OD_MODULE_NAME}
	    ${OD_LIB_DEP_LIBS}
	    ${OD_MODULE_EXTERNAL_LIBS}
	    ${OD_MODULE_EXTERNAL_SYSLIBS} )
    OD_SET_UNICODE( ${OD_MODULE_NAME} )
	
    set ( TARGET_PROPERTIES ${OD_MODULE_NAME}
	  PROPERTIES
	  LABELS ${OD_MODULE_NAME}
	  ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_OUTPUT_DIR}/${OD_ARCHIVE_DIRECTORY}"
	  LIBRARY_OUTPUT_DIRECTORY "${PROJECT_OUTPUT_DIR}/${OD_LIBRARY_DIRECTORY}"
	  RUNTIME_OUTPUT_DIRECTORY "${PROJECT_OUTPUT_DIR}/${OD_RUNTIME_DIRECTORY}"
	  PDB_OUTPUT_DIRECTORY "${PROJECT_OUTPUT_DIR}/${OD_PDB_DIRECTORY}" )

    if ( DEFINED OD_FOLDER )
	list ( APPEND TARGET_PROPERTIES
	       FOLDER "${OD_FOLDER}" )
    endif()

    if ( WIN32 ) #Would be nice to use on Unix too, to get libBasic.so.7.1.0 ...
	list ( APPEND TARGET_PROPERTIES
	       VERSION ${OD_BUILD_VERSION}
	       SOVERSION ${OD_API_VERSION} )
    endif()

    ADD_TARGET_PROPERTIES( ${OD_MODULE_NAME} )
    OD_GENERATE_SYMBOLS( ${OD_MODULE_NAME} )

    install( TARGETS ${OD_MODULE_NAME}
	     ARCHIVE DESTINATION "${OD_ARCHIVE_DIRECTORY}"
	     LIBRARY DESTINATION "${OD_LIBRARY_DIRECTORY}"
	     RUNTIME DESTINATION "${OD_LOCATION_DIRECTORY}" )
    if ( WIN32 )
	install( FILES "$<TARGET_PDB_FILE:${OD_MODULE_NAME}>"
		 DESTINATION "${OD_LOCATION_DIRECTORY}" )
    endif()

    if ( DEFINED OD_MODULE_ALL_EXTERNAL_LIBS )
	OD_INSTALL_DEPENDENCIES( "${OD_MODULE_ALL_EXTERNAL_LIBS}" )
    endif()

    #Add to list of all files
    OD_ADD_SOURCE_FILES( ${OD_MODULE_SOURCES} ${OD_MODULE_INCFILES} )

endif ( OD_MODULE_HAS_LIBRARY )

OD_ADD_SOURCE_FILES( CMakeLists.txt )

#Setup common things for batch-programs
if( OD_MODULE_BATCHPROGS OR OD_BATCH_TEST_PROGS )
    set ( OD_USEBATCH 1 )
endif()

set ( OD_RUNTIMELIBS ${OD_MODULE_DEPS})
if ( OD_MODULE_HAS_LIBRARY )
    list ( APPEND OD_RUNTIMELIBS ${OD_MODULE_NAME} )
endif ( OD_MODULE_HAS_LIBRARY )

#Add executable targets
if( OD_MODULE_TESTPROGS OR OD_MODULE_PROGS OR OD_MODULE_GUI_PROGS OR OD_ELEVATED_PERMISSIONS_PROGS OR OD_ELEVATED_PERMISSIONS_GUI_PROGS )

    foreach ( EXEC ${OD_MODULE_TESTPROGS} ${OD_MODULE_PROGS} ${OD_MODULE_GUI_PROGS} ${OD_ELEVATED_PERMISSIONS_PROGS} ${OD_ELEVATED_PERMISSIONS_GUI_PROGS} )
    	get_filename_component( TARGET_NAME ${EXEC} NAME_WE )

	if ( DEFINED RC_${TARGET_NAME}_DESC )
	    set( RC_APP_NAME ${TARGET_NAME} )
	    set( RC_APP_DESC ${RC_${TARGET_NAME}_DESC} )
	    set( RC_ICO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/resources" )
	    configure_file( "${OpendTect_DIR}/CMakeModules/templates/od_app.rc.in" "${CMAKE_CURRENT_BINARY_DIR}/resources/${TARGET_NAME}.rc" )
	    configure_file( "${OpendTect_DIR}/CMakeModules/templates/resource.h.in" "${CMAKE_CURRENT_BINARY_DIR}/resources/resource.h" )
	    list( APPEND OD_${TARGET_NAME}_RESOURCE resources/${TARGET_NAME}.rc resources/resource.h )
	endif()

	OD_ADD_SOURCE_FILES( ${EXEC} ${OD_${TARGET_NAME}_RESOURCE} )

	#Check if from GUI list
	if ( EXEC IN_LIST OD_MODULE_GUI_PRODS OR
	     EXEC IN_LIST OD_ELEVATED_PERMISSIONS_GUI_PROGS )
	    set( OD_EXEC_GUI_SYSTEM ${OD_GUI_SYSTEM} )
	endif()
	if ( EXEC IN_LIST OD_MODULE_TESTPROGS )
	    set( OD_EXEC_FOLDER "Testing" )
	else()
	    if ( DEFINED OD_FOLDER )
	        set( OD_EXEC_FOLDER ${OD_FOLDER} )
	    endif()
	    list ( APPEND OD_${OD_MODULE_NAME}_PROGS ${TARGET_NAME} )
	endif()

	add_executable( ${TARGET_NAME} ${OD_EXEC_GUI_SYSTEM} ${EXEC} 
			${OD_${TARGET_NAME}_RESOURCE} )
	OD_SET_UNICODE( ${TARGET_NAME} )
	if ( OD_EXECUTABLE_COMPILE_FLAGS )
	    set_source_files_properties( ${EXEC} PROPERTIES
			COMPILE_FLAGS ${OD_EXECUTABLE_COMPILE_FLAGS} )
	endif( OD_EXECUTABLE_COMPILE_FLAGS )

	#Check if from Elevated permission list
	if ( WIN32 AND (EXEC IN_LIST OD_ELEVATED_PERMISSIONS_PROGS OR
			EXEC IN_LIST OD_ELEVATED_PERMISSIONS_GUI_PROGS) )
	    list ( APPEND OD_MODULE_LINK_OPTIONS "${OD_UAC_LINKFLAGS}" )
	endif()
			
	set( TARGET_PROPERTIES ${TARGET_NAME}
	     PROPERTIES
	     LABELS ${OD_MODULE_NAME}
	     ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_OUTPUT_DIR}/${OD_ARCHIVE_DIRECTORY}"
	     RUNTIME_OUTPUT_DIRECTORY "${PROJECT_OUTPUT_DIR}/${OD_RUNTIME_DIRECTORY}"
	     PDB_OUTPUT_DIRECTORY "${PROJECT_OUTPUT_DIR}/${OD_PDB_DIRECTORY}" )

	if ( APPLE )
	    list ( APPEND TARGET_PROPERTIES
		   INSTALL_RPATH "\@executable_path/../Frameworks" )
	endif()

	if ( DEFINED OD_EXEC_FOLDER )
	    list ( APPEND TARGET_PROPERTIES
		   FOLDER ${OD_EXEC_FOLDER} )
	endif()

	if ( WIN32 )
	    list ( APPEND TARGET_PROPERTIES
		   VERSION ${OD_BUILD_VERSION} )
	endif()

	if ( WIN32 AND OD_ENABLE_BREAKPAD AND NOT "${OD_EXEC_GUI_SYSTEM}" STREQUAL "" AND
	     (OD_MODULE_GUI_PROGS OR OD_ELEVATED_PERMISSIONS_GUI_PROGS) )
	    # Qt does not provide qtmain.pdb
	    list ( APPEND OD_MODULE_LINK_OPTIONS "$<$<NOT:$<CONFIG:Debug>>:/ignore:4099>" )
	    list ( REMOVE_DUPLICATES OD_MODULE_LINK_OPTIONS )
	endif()

	ADD_TARGET_PROPERTIES( ${TARGET_NAME} )

	target_link_libraries(
	    ${TARGET_NAME}
	    ${OD_EXEC_DEP_LIBS}
	    ${OD_RUNTIMELIBS} )
        if( OD_CREATE_LAUNCHERS )
	    create_target_launcher( ${TARGET_NAME}
		RUNTIME_LIBRARY_DIRS
		${OD_MODULE_RUNTIMEPATH}
		"${CMAKE_BINARY_DIR}/${OD_RUNTIME_DIRECTORY}"
		ENVIRONMENT
		WORK=${PROJECT_OUTPUT_DIR})
        endif( OD_CREATE_LAUNCHERS )
	install( TARGETS ${TARGET_NAME}
		 RUNTIME DESTINATION "${OD_RUNTIME_DIRECTORY}" )
	if ( WIN32 )
	    install( FILES "$<TARGET_PDB_FILE:${TARGET_NAME}>"
		     DESTINATION "${OD_RUNTIME_DIRECTORY}" )
	endif()

	OD_GENERATE_SYMBOLS( ${TARGET_NAME} )
    endforeach()

endif()

if( OD_MODULE_BATCHPROGS )
    #Add dep on Batch if there are batch-progs
    set ( PROGRAM_RUNTIMELIBS ${OD_RUNTIMELIBS} "Batch" "Network" )
    OD_ADD_SOURCE_FILES( ${OD_MODULE_BATCHPROGS} )

    foreach ( EXEC ${OD_MODULE_BATCHPROGS} )
	get_filename_component( TARGET_NAME ${EXEC} NAME_WE )
	list ( APPEND OD_${OD_MODULE_NAME}_PROGS ${TARGET_NAME} )
	add_executable( ${TARGET_NAME} ${EXEC} )
	OD_SET_UNICODE( ${TARGET_NAME} )
	if ( OD_EXECUTABLE_COMPILE_FLAGS )
	    set_source_files_properties( ${EXEC} PROPERTIES
			COMPILE_FLAGS ${OD_EXECUTABLE_COMPILE_FLAGS} )
	endif( OD_EXECUTABLE_COMPILE_FLAGS )
	target_link_libraries(
	    ${TARGET_NAME}
	    ${OD_EXEC_DEP_LIBS}
	    ${PROGRAM_RUNTIMELIBS} )

	set( TARGET_PROPERTIES ${TARGET_NAME}
	     PROPERTIES
	     LABELS ${OD_MODULE_NAME}
	     ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_OUTPUT_DIR}/${OD_ARCHIVE_DIRECTORY}"
	     RUNTIME_OUTPUT_DIRECTORY "${PROJECT_OUTPUT_DIR}/${OD_RUNTIME_DIRECTORY}"
	     PDB_OUTPUT_DIRECTORY "${PROJECT_OUTPUT_DIR}/${OD_PDB_DIRECTORY}" )

	if ( APPLE )
	    list ( APPEND TARGET_PROPERTIES
		   INSTALL_RPATH "\@executable_path/../Frameworks" )
	endif()

	if ( DEFINED OD_FOLDER )
	    list ( APPEND TARGET_PROPERTIES
		   FOLDER ${OD_FOLDER} )
	endif()

	if ( WIN32 )
	    list ( APPEND TARGET_PROPERTIES
		   VERSION ${OD_BUILD_VERSION} )
	endif()
	if ( NOT __prog__ IN_LIST OD_MODULE_COMPILE_DEFINITIONS )
	    list ( APPEND OD_MODULE_COMPILE_DEFINITIONS __prog__ )
	endif()

	ADD_TARGET_PROPERTIES( ${TARGET_NAME} )

	if( OD_CREATE_LAUNCHERS )
	    create_target_launcher( ${TARGET_NAME}
		RUNTIME_LIBRARY_DIRS
		${OD_MODULE_RUNTIMEPATH} )
	endif( OD_CREATE_LAUNCHERS )
	install( TARGETS ${TARGET_NAME}
		 RUNTIME DESTINATION "${OD_RUNTIME_DIRECTORY}" )
	if ( WIN32 )
	    install( FILES "$<TARGET_PDB_FILE:${TARGET_NAME}>"
		     DESTINATION "${OD_RUNTIME_DIRECTORY}" )
	endif()

	OD_GENERATE_SYMBOLS( ${TARGET_NAME} )
    endforeach()

endif( OD_MODULE_BATCHPROGS )

if ( NOT "${OD_${OD_MODULE_NAME}_PROGS}" STREQUAL "" )
    set( OD_${OD_MODULE_NAME}_PROGS
	 ${OD_${OD_MODULE_NAME}_PROGS} PARENT_SCOPE )
endif()

if ( NOT DEFINED OD_MODULE_TESTS_IGNORE_CONTINUOUS )
    set ( OD_MODULE_TESTS_IGNORE_CONTINUOUS "" )
endif()
if ( NOT DEFINED OD_MODULE_TESTS_IGNORE_EXPERIMENTAL )
    set ( OD_MODULE_TESTS_IGNORE_EXPERIMENTAL "" )
endif()

foreach ( TEST_FILE ${OD_NIGHTLY_TEST_PROGS} ${OD_BATCH_TEST_PROGS} )
    get_filename_component( TEST_NAME ${TEST_FILE} NAME_WE )
    set ( TEST_NAME test_${TEST_NAME} )

    set ( OD_MODULE_TESTS_IGNORE_CONTINUOUS ${OD_MODULE_TESTS_IGNORE_CONTINUOUS} ${TEST_NAME} )
    set ( OD_MODULE_TESTS_IGNORE_EXPERIMENTAL ${OD_MODULE_TESTS_IGNORE_EXPERIMENTAL} ${TEST_NAME} )
endforeach()

set ( OD_TESTS_IGNORE_CONTINUOUS ${OD_TESTS_IGNORE_CONTINUOUS} ${OD_MODULE_TESTS_IGNORE_CONTINUOUS} PARENT_SCOPE )
set ( OD_TESTS_IGNORE_EXPERIMENTAL ${OD_TESTS_IGNORE_EXPERIMENTAL} ${OD_MODULE_TESTS_IGNORE_EXPERIMENTAL} PARENT_SCOPE )

foreach ( TEST_FILE ${OD_TEST_PROGS} ${OD_BATCH_TEST_PROGS} ${OD_NIGHTLY_TEST_PROGS} )

    set ( PROGRAM_RUNTIMELIBS ${OD_RUNTIMELIBS} )

    #Add dep on Batch if there are batch-progs
    if ( OD_BATCH_TEST_PROGS AND ${TEST_FILE} IN_LIST OD_BATCH_TEST_PROGS )
	list ( APPEND PROGRAM_RUNTIMELIBS "Batch" "Network" )
	if ( NOT __prog__ IN_LIST OD_MODULE_COMPILE_DEFINITIONS )
	    list ( APPEND OD_MODULE_COMPILE_DEFINITIONS __prog__ )
	endif()
    endif()

    get_filename_component( TEST_NAME ${TEST_FILE} NAME_WE )
    set ( PARAMETER_FILE ${CMAKE_CURRENT_SOURCE_DIR}/tests/${TEST_NAME}.par )
    set ( TEST_NAME test_${TEST_NAME} )
    add_executable( ${TEST_NAME} ${OD_EXEC_GUI_SYSTEM} tests/${TEST_FILE} )
    OD_SET_UNICODE( ${TEST_NAME} )
    if ( OD_EXECUTABLE_COMPILE_FLAGS )
	set_source_files_properties( tests/${TEST_FILE} PROPERTIES
			COMPILE_FLAGS ${OD_EXECUTABLE_COMPILE_FLAGS} )
    endif( OD_EXECUTABLE_COMPILE_FLAGS )
    OD_ADD_SOURCE_FILES( tests/${TEST_FILE} )
    set( TARGET_PROPERTIES ${TEST_NAME}
	    PROPERTIES
	    LABELS ${OD_MODULE_NAME}
	    FOLDER "Testing"
	    ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_OUTPUT_DIR}/${OD_ARCHIVE_DIRECTORY}"
	    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_OUTPUT_DIR}/${OD_RUNTIME_DIRECTORY}"
	    PDB_OUTPUT_DIRECTORY "${PROJECT_OUTPUT_DIR}/${OD_PDB_DIRECTORY}" )

    if ( APPLE )
	list ( APPEND TARGET_PROPERTIES
	       INSTALL_RPATH "\@executable_path/../Frameworks" )
    endif()

    ADD_TARGET_PROPERTIES( ${TEST_NAME} )

    target_link_libraries(
	    ${TEST_NAME}
	    ${OD_EXEC_DEP_LIBS}
	    ${PROGRAM_RUNTIMELIBS} )

    if( OD_CREATE_LAUNCHERS )
	    create_target_launcher( ${TEST_NAME}
		RUNTIME_LIBRARY_DIRS
		${OD_MODULE_RUNTIMEPATH}
		"${CMAKE_BINARY_DIR}/${OD_RUNTIME_DIRECTORY}"
		ENVIRONMENT
		WORK=${PROJECT_OUTPUT_DIR})
    endif( OD_CREATE_LAUNCHERS )
    install( TARGETS ${TEST_NAME}
	     RUNTIME DESTINATION "${OD_RUNTIME_DIRECTORY}" )
    if ( WIN32 )
	install( FILES "$<TARGET_PDB_FILE:${TEST_NAME}>"
		 DESTINATION "${OD_RUNTIME_DIRECTORY}" )
    endif()

    ADD_TEST_PROGRAM( ${TEST_NAME} )
    set_property( TEST ${TEST_NAME} PROPERTY ${OD_MODULE_NAME} )
    unset( PARAMETER_FILE )
    unset ( TEST_NAME )
endforeach()


if( OD_USEBATCH )
    list(APPEND OD_MODULE_INCLUDEPATH
		${OD_Batch_INCLUDEPATH}
		${OD_Network_INCLUDEPATH}
		${OD_BINARY_BASEDIR}/include/Batch
		${OD_BINARY_BASEDIR}/include/Network)
endif( OD_USEBATCH )

#Set current include_path
include_directories( SYSTEM ${OD_MODULE_INCLUDESYSPATH} )
include_directories( ${OD_MODULE_INCLUDEPATH} )

if ( WIN32 AND (OD_${OD_MODULE_NAME}_EXTERNAL_LIBS OR
	        OD_${OD_MODULE_NAME}_EXTERNAL_RUNTIME_LIBS ))
    list ( APPEND EXTLIBS ${OD_${OD_MODULE_NAME}_EXTERNAL_LIBS} )
    list ( APPEND EXTLIBS ${OD_${OD_MODULE_NAME}_EXTERNAL_RUNTIME_LIBS} )
    list ( LENGTH EXTLIBS EXTLIBS_SIZE )

    if ( NOT ${EXTLIBS_SIZE} EQUAL -1 AND OD_IS_PLUGIN )
	get_filename_component( QTDIR "${QT_DIR}/../../../" REALPATH )
	foreach( TRGT ${EXTLIBS} )
	    if ( NOT TARGET ${TRGT} OR
		 "${TRGT}" MATCHES ".*Qt${QT_VERSION_MAJOR}.*" OR
		 "${TRGT}" MATCHES "^${QTDIR}" )
		continue()
	    endif()
	    get_target_property( DEP_LIB_TYPE ${TRGT} TYPE )
	    if ( DEP_LIB_TYPE AND (${DEP_LIB_TYPE} STREQUAL "SHARED_LIBRARY" OR
				   ${DEP_LIB_TYPE} STREQUAL "UNKNOWN_LIBRARY") )
		add_custom_command( TARGET ${OD_MODULE_NAME} POST_BUILD
		    COMMAND ${CMAKE_COMMAND} -E copy_if_different
			    "$<TARGET_FILE:${TRGT}>"
			    "${PROJECT_OUTPUT_DIR}/${OD_RUNTIME_DIRECTORY}"
		    COMMENT "\nCopying external DLLs of ${TRGT} for the plugin ${OD_MODULE_NAME}" )
	    endif()
	    unset( DEP_LIB_TYPE )
	endforeach()
    endif()

endif()

endmacro( OD_INIT_MODULE )

# OD_GET_ALL_DEPS( MODULE LISTNAME ) - dumps all deps to MODULE into LISTNAME
macro( OD_GET_ALL_DEPS MODULE DEPS )
    foreach ( DEPLIB ${OD_${MODULE}_DEPS} )
	OD_GET_ALL_DEPS_ADD( ${DEPLIB} ${DEPS} )
    endforeach()
endmacro( OD_GET_ALL_DEPS )

macro( OD_GET_ALL_DEPS_ADD DEP DEPLIST )
    if ( NOT ${DEP} IN_LIST ${DEPLIST} )
        list ( APPEND ${DEPLIST} ${DEP} )
        foreach ( DEPLIB ${OD_${DEP}_DEPS} )
            OD_GET_ALL_DEPS_ADD( ${DEPLIB} ${DEPLIST} )
        endforeach()
    endif()
endmacro ( OD_GET_ALL_DEPS_ADD )



# OD_ADD_PLUGIN_SOURCES(SOURCES) - Adds sources in a submodule of a plugin
#
# Input variables:
# 
# OD_PLUGINSUBDIR			: Name sub-module of the plugin
# SOURCES				: List of sources to add
#
# Output:
# OD_MODULE_SOURCES

macro ( OD_ADD_PLUGIN_SOURCES )
    foreach ( SRC ${ARGV} )
	list( APPEND OD_MODULE_SOURCES src/${OD_PLUGINSUBDIR}/${SRC} )
    endforeach()
endmacro()

# OD_ADD_PLUGIN_EXECS(SOURCES) - Adds sources in a submodule of a plugin
#
# Input variables:
# 
# OD_PLUGINSUBDIR			: Name sub-module of the plugin
# SOURCES				: List of sources to add
#
# Output:
# OD_MODULE_PROGS

macro ( OD_ADD_PLUGIN_EXECS )
    foreach ( SRC ${ARGV} )
	list( APPEND OD_MODULE_PROGS src/${OD_PLUGINSUBDIR}/${SRC} )
    endforeach()
endmacro()


# OD_ADD_PLUGIN_BATCHPROGS(SOURCES) - Adds sources in a submodule of a plugin
#
# Input variables:
# 
# OD_PLUGINSUBDIR			: Name sub-module of the plugin
# SOURCES				: List of sources to add
#
# Output:
# OD_MODULE_BATCHPROGS

macro ( OD_ADD_PLUGIN_BATCHPROGS )
    foreach ( SRC ${ARGV} )
	list( APPEND OD_MODULE_BATCHPROGS src/${OD_PLUGINSUBDIR}/${SRC} )
    endforeach()
endmacro()

#Adds lists of files to global file-list
macro ( OD_ADD_SOURCE_FILES )
    foreach ( THEFILE ${ARGV} )
	get_filename_component( PATH ${THEFILE} ABSOLUTE )
	file ( RELATIVE_PATH RELPATH "${CMAKE_BINARY_DIR}" ${PATH} ) 
	file ( APPEND ${OD_SOURCELIST_FILE} ${RELPATH} "\n" )
    endforeach()
endmacro()

macro( FIND_OD_PLUGIN NAME )
  list( FIND OD_MODULE_NAMES_${OD_SUBSYSTEM} ${NAME} _index )
  if ( NOT ${_index} EQUAL -1 )
    set( ${NAME}_FOUND TRUE )
  endif()
endmacro()

macro( OD_EXPORT_SETUP )
   foreach( SETUPNM ${SETUPNMS} )
	if( DEFINED OD_${SETUPNM} )
	    set( OD_${OD_MODULE_NAME}_SETUP_${SETUPNM}
		    ${OD_${SETUPNM}} PARENT_SCOPE )
	elseif ( ${OD_${SETUPNM}} )
	    set( OD_${OD_MODULE_NAME}_SETUP_${SETUPNM}
		    ${OD_${SETUPNM}} PARENT_SCOPE )
	endif()
   endforeach()
endmacro()

macro( ADD_TARGET_PROPERTIES TARGETNM )

    if ( CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" )
	list ( APPEND TARGET_PROPERTIES
	       MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" )
    endif()

    set_target_properties( ${TARGET_PROPERTIES} )

    if ( OD_MODULE_COMPILE_DEFINITIONS )
	target_compile_definitions( ${TARGETNM} PRIVATE
				    ${OD_MODULE_COMPILE_DEFINITIONS} )
    endif()
    if ( OD_MODULE_COMPILE_OPTIONS )
	target_compile_options( ${TARGETNM} PRIVATE
				${OD_PLATFORM_COMPILE_OPTIONS}
				${OD_MODULE_COMPILE_OPTIONS} )
    endif()
    if ( OD_MODULE_LINK_OPTIONS )
	target_link_options( ${TARGETNM} PRIVATE
			     ${OD_MODULE_LINK_OPTIONS} )
    endif()

endmacro()
