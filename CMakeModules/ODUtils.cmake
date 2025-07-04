#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

function( get_buildinsrc REQUIRED_ARG )
    if ( NOT "${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}" )
	get_filename_component( SRCDIR "${CMAKE_SOURCE_DIR}" REALPATH )
	get_filename_component( BINDIR "${CMAKE_BINARY_DIR}" REALPATH )
	if ( NOT "${SRCDIR}" STREQUAL "${BINDIR}" )
	    set(${REQUIRED_ARG} False PARENT_SCOPE )
	else()
	    set(${REQUIRED_ARG} True PARENT_SCOPE )
	endif()
    else()
	set(${REQUIRED_ARG} True PARENT_SCOPE )
    endif()
endfunction( get_buildinsrc )

get_buildinsrc( BUILDINSRC )

if ( APPLE )
    set ( MISC_INSTALL_PREFIX Contents/Resources )
else()
    set ( MISC_INSTALL_PREFIX . )
endif()
set ( OD_CMAKE_MODULES_DIR "${OD_BINARY_BASEDIR}/${MISC_INSTALL_PREFIX}/CMakeModules" )

set ( OD_SOURCELIST_FILE "${CMAKE_BINARY_DIR}/Testing/sourcefiles_${OD_SUBSYSTEM}.txt" )
file ( REMOVE ${OD_SOURCELIST_FILE} )

# As per cmake naming:
# LIBRARY: link libraries directory
# LOCATION: runtime libraries directory
# RUNTIME: executables directory
if ( APPLE )
    set ( OD_ARCHIVE_DIRECTORY "Contents/Frameworks$<$<CONFIG:Debug>:/Debug>" )
    set ( OD_LIBRARY_DIRECTORY "Contents/Frameworks$<$<CONFIG:Debug>:/Debug>" )
    set ( OD_RUNTIME_DIRECTORY "Contents/MacOS$<$<CONFIG:Debug>:/Debug>" )
    set ( OD_PDB_DIRECTORY "Contents/MacOS$<$<CONFIG:Debug>:/Debug>" ) # not really used
else()
    set ( OD_ARCHIVE_DIRECTORY "bin/${OD_PLFSUBDIR}/$<IF:$<CONFIG:Debug>,Debug,Release>" )
    set ( OD_LIBRARY_DIRECTORY "bin/${OD_PLFSUBDIR}/$<IF:$<CONFIG:Debug>,Debug,Release>" )
    set ( OD_RUNTIME_DIRECTORY "bin/${OD_PLFSUBDIR}/$<IF:$<CONFIG:Debug>,Debug,Release>" )
    set ( OD_PDB_DIRECTORY "bin/${OD_PLFSUBDIR}/$<IF:$<CONFIG:Debug>,Debug,Release>" )
endif()
if ( WIN32 )
    set ( OD_LOCATION_DIRECTORY "${OD_RUNTIME_DIRECTORY}" )
else()
    set ( OD_LOCATION_DIRECTORY "${OD_LIBRARY_DIRECTORY}" )
endif()

set ( OD_DATA_INSTALL_RELPATH "${MISC_INSTALL_PREFIX}/share" )

set ( OD_BUILD_VERSION "${OpendTect_VERSION_MAJOR}.${OpendTect_VERSION_MINOR}.${OpendTect_VERSION_PATCH}")
set ( OD_API_VERSION "${OpendTect_VERSION_MAJOR}.${OpendTect_VERSION_MINOR}.${OpendTect_VERSION_PATCH}" )

set ( OD_MAIN_EXEC od_main )
if ( WIN32 )
    list( APPEND OD_MAIN_EXEC od_main_console )
endif()
set ( OD_ATTRIB_EXECS od_process_attrib od_process_attrib_em od_stratamp )
set ( OD_VOLUME_EXECS od_process_volume )
set ( OD_SEIS_EXECS od_cbvs_browse od_copy_seis od_process_2dto3d od_process_segyio )
set ( OD_DB_EXECS ODBind )
set ( OD_SURVEY_EXECS od_SEGYExaminer )
set ( OD_PRESTACK_EXECS od_process_prestack )
set ( OD_ZAXISTRANSFORM_EXECS od_process_time2depth )

#Should not be here.
set ( DGB_PRO_EXECS od_petrelhortransfer od_petrelseistransfer od_fullpetrelsurveyimp od_Basemap od_LogViewer )
set ( DGB_ML_EXECS od_deeplearn_apply )
set ( DGB_SEGY_EXECS od_Basemap od_DeepLearning od_DeepLearning_CC od_SynthRock )
set ( DGB_SR_EXECS od_SynthRock )
set ( DGB_ML_UIEXECS od_DeepLearning od_DeepLearning_CC od_DeepLearning_EM od_DeepLearning_TM od_DeepLearning_ModelImport )

option ( OD_DISABLE_EXTERNAL_LIBS_CHECK "Disabling automatic retrieval of external dependency for plugins" ON )
mark_as_advanced( FORCE OD_DISABLE_EXTERNAL_LIBS_CHECK )

macro( OD_ADD_EXTERNALS )
    if ( APPLE )
	set( EXTPLFARCH "-DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}" )
	if ( NOT ${CMAKE_OSX_DEPLOYMENT_TARGET} STREQUAL "" )
	    set( EXTPLFTOOLSET "-DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET}" )
	endif()
    endif()
    if ( CMAKE_GENERATOR MATCHES "Ninja" )
	if ( UNIX AND EXISTS "${CMAKE_MAKE_PROGRAM}" )
	    set( EXTMAKEPROG "-DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}" )
	endif()
	set( EXTGENERATOR "-GNinja Multi-Config" )
    elseif ( WIN32 AND CMAKE_GENERATOR MATCHES "Visual Studio" )
	set( EXTGENERATOR "-G ${CMAKE_GENERATOR}" )
	if ( NOT "${CMAKE_VS_PLATFORM_NAME_DEFAULT}" STREQUAL "" )
	    set( EXTPLFARCH "-A ${CMAKE_VS_PLATFORM_NAME_DEFAULT}" )
	endif()
	set( EXTPLFTOOLSET "-T v${MSVC_TOOLSET_VERSION}" )
    else()
	find_program( NINJA_BIN "ninja"
		PATHS "${QT_DIR}" "${Qt5Core_Dir}" "${Qt6Core_Dir}"
		PATH_SUFFIXES "../../Tools/Ninja"
			      "../../../../../Tools/Ninja" )
	if ( EXISTS "${NINJA_BIN}" )
	    set( EXTGENERATOR "-GNinja Multi-Config" )
	    set( EXTMAKEPROG "-DCMAKE_MAKE_PROGRAM=${NINJA_BIN}" )
	endif()
    endif()
    OD_ADD_BREAKPAD()
    if ( WIN32 ) #Keep before Qt and before osg(Geo)
	OD_FIND_ZLIB()
    else()
	OD_FIND_Fontconfig() # includes ZLIB
    endif()
    OD_FIND_OPENSSL()
    OD_ADD_QT()
    OD_ADD_OSGGEO()
    OD_FIND_OSG()
    OD_FIND_PROJ()
    OD_FIND_HDF5()
    unset( NINJA_BIN CACHE )
endmacro(OD_ADD_EXTERNALS)

#Macro for going through a list of modules and adding them
macro ( OD_ADD_MODULES )
    set( DIR ${ARGV0} )
    cmake_path( IS_RELATIVE DIR ISRELDIR )
    if ( ${ISRELDIR} )
	set( MODULE_OUTPUT_DIR "${DIR}" )
    else()
	if ( DIR MATCHES "^${CMAKE_SOURCE_DIR}/external" )
	    string( REPLACE "${CMAKE_SOURCE_DIR}/" "" MODULE_OUTPUT_DIR "${DIR}" )
	else()
	    if ( "${PLUGIN_DIR}" STREQUAL "" )
		set( MODULE_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}" )
	    else()
		cmake_path( REMOVE_FILENAME PLUGIN_DIR OUTPUT_VARIABLE PROJECT_ROOT )
		if ( DIR MATCHES "^${PROJECT_ROOT}" )
		    string( REPLACE "${PROJECT_ROOT}" "" MODULE_OUTPUT_DIR "${DIR}" )
		else()
		    message( AUTHOR_WARNING "Should not be reached" )
		    set( MODULE_OUTPUT_DIR "${DIR}" )
		endif()
		unset( PROJECT_ROOT )
	    endif()
	endif()
    endif()
    unset( ISRELDIR )
    string(TIMESTAMP YEAR %Y)
    foreach( OD_MODULE_NAME ${ARGV} )
	if ( NOT ${OD_MODULE_NAME} STREQUAL "${DIR}" )
	    add_subdirectory( "${DIR}/${OD_MODULE_NAME}"
			      "${MODULE_OUTPUT_DIR}/${OD_MODULE_NAME}" )
	endif()
    endforeach()
    unset( MODULE_OUTPUT_DIR )
endmacro(OD_ADD_MODULES)

# Macro for going through a list of modules and adding them
# as optional targets
macro ( OD_ADD_OPTIONAL_MODULES )
    set( DIR ${ARGV0} )
    cmake_path( IS_RELATIVE DIR ISRELDIR )
    if ( ${ISRELDIR} )
	set( MODULE_OUTPUT_DIR "${DIR}" )
    else()
	if ( DIR MATCHES "^${CMAKE_SOURCE_DIR}/external" )
	    string( REPLACE "${CMAKE_SOURCE_DIR}/" "" MODULE_OUTPUT_DIR "${DIR}" )
	else()
	    cmake_path( REMOVE_FILENAME PLUGIN_DIR OUTPUT_VARIABLE PROJECT_ROOT )
	    if ( DIR MATCHES "^${PROJECT_ROOT}" )
		string( REPLACE "${PROJECT_ROOT}" "" MODULE_OUTPUT_DIR "${DIR}" )
	    else()
		message( AUTHOR_WARNING "Should not be reached" )
		set( MODULE_OUTPUT_DIR "${DIR}" )
	    endif()
	    unset( PROJECT_ROOT )
	endif()
    endif()
    unset( ISRELDIR )
    foreach( OD_MODULE_NAME ${ARGV} )
	if ( NOT ${OD_MODULE_NAME} STREQUAL "${DIR}" )
	    add_subdirectory( "${DIR}/${OD_MODULE_NAME}"
			      "${MODULE_OUTPUT_DIR}/${OD_MODULE_NAME}"
			      EXCLUDE_FROM_ALL )
	endif()
    endforeach()
    unset( MODULE_OUTPUT_DIR )
endmacro(OD_ADD_OPTIONAL_MODULES)

function( get_link_libraries OUTPUT_LIST TARGET )

    get_target_property(IMPORTED ${TARGET} IMPORTED)
    list ( APPEND VISITED_TARGETS ${TARGET} )
    if ( IMPORTED )
	get_target_property( LIBS ${TARGET} INTERFACE_LINK_LIBRARIES )
    else()
	get_target_property( LIBS ${TARGET} LINK_LIBRARIES )
    endif()
    set( LIB_LIST "" )
    foreach( LIB ${LIBS} )
	if ( TARGET ${LIB} )
	    list ( FIND VISITED_TARGETS ${LIB} VISITED )
	    if ( ${VISITED} EQUAL -1 )
		get_link_libraries( LINK_LIB_LIST ${LIB} )
		list ( APPEND LIB_LIST ${LIB} ${LINK_LIB_LIST} )
	    endif()
	endif()
    endforeach()
    set( VISISTED_TARGETS ${VISITED_TARGETS} PARENT_SCOPE )
    set( ${OUTPUT_LIST} ${LIB_LIST} PARENT_SCOPE )

endfunction()

function( get_shared_link_libraries OUTPUT_LIST TARGETS )
    foreach( LIB ${TARGETS} )
	get_target_property( DEPLIBTYPE ${LIB} TYPE )
	if ( "${DEPLIBTYPE}" STREQUAL "SHARED_LIBRARY" )
	    list ( APPEND LIB_LIST ${LIB} )
	endif()
    endforeach()
    set( ${OUTPUT_LIST} ${LIB_LIST} PARENT_SCOPE )
endfunction(get_shared_link_libraries)

function(guess_runtime_library_dirs _var)
    foreach(_lib ${ARGN})
	if ( EXISTS "${_lib}" )
	    message( AUTHOR_WARNING "Should not be used, use a target" )
	    get_filename_component( _libdir "${_lib}" DIRECTORY )
	    if ( WIN32 )
		get_filename_component( _bindir "${_libdir}" DIRECTORY )
		set( _bindir "${_bindir}/bin" )
		if ( IS_DIRECTORY "${_bindir}" )
		    list( APPEND _libdirs "${_bindir}" )
		else()
		    message( AUTHOR_WARNING "Cannot find target directory for ${_lib}" )
		endif()
	    else()
		list( APPEND _libdirs "${_libdir}" )
	    endif()
	elseif( TARGET "${_lib}" )
	    get_target_property( TARGET_TYPE ${_lib} TYPE )
	    if ( TARGET_TYPE AND ${TARGET_TYPE} STREQUAL "SHARED_LIBRARY" )
		list( APPEND _libdirs "$<TARGET_FILE_DIR:${_lib}>" )
	    endif()
	    unset( TARGET_TYPE )
	endif()
    endforeach()
    list( REMOVE_DUPLICATES _libdirs )
    set(${_var} "${_libdirs}" PARENT_SCOPE)
endfunction()

function(od_get_dll _lib _dll)
    get_filename_component( DLLBASEPATH "${_lib}" DIRECTORY )
    get_filename_component( DLLBASEPATH "${DLLBASEPATH}" DIRECTORY )
    if ( IS_DIRECTORY "${DLLBASEPATH}/bin" )
	set( DLLPATH "${DLLBASEPATH}/bin" )
    elseif ( IS_DIRECTORY "${DLLBASEPATH}/../bin/Debug" )
	get_filename_component( DLLPATH "${DLLBASEPATH}/../bin/Debug" REALPATH )
    endif()
    unset( DLLBASEPATH )
    if ( IS_DIRECTORY "${DLLPATH}" )
	get_filename_component( BASEFILENM "${_lib}" NAME_WE )
	file( GLOB DLLFILENM "${DLLPATH}/*${BASEFILENM}.dll" )
	if ( NOT EXISTS "${DLLFILENM}" )
	    file( GLOB DLLFILENM "${DLLPATH}/*${BASEFILENM}*x64.dll" )
	endif()
	unset( DLLPATH )
	unset( BASEFILENM )
	if ( EXISTS "${DLLFILENM}" )
	    set( ${_dll} "${DLLFILENM}" PARENT_SCOPE )
	    unset( DLLFILENM )
	    return()
	endif()
    endif()
    unset( DLLPATH )
    message( FATAL_ERROR "Cannot find DLL for library ${_lib}" )
endfunction()

function(od_map_configurations _trgt)
    get_target_property( TRGT_CONFIGS ${_trgt} IMPORTED_CONFIGURATIONS )
    if ( NOT TRGT_CONFIGS )
	return()
    endif()

    set( _tconfigs RELWITHDEBINFO RELEASE )
    set( _rconfigs RELEASE RELWITHDEBINFO )
    foreach( _tconfig _rconfigs IN ZIP_LISTS _tconfigs _rconfigs )
	if ( ${_tconfig} IN_LIST TRGT_CONFIGS )
	    if ( NOT DEBUG IN_LIST TRGT_CONFIGS )
		set_target_properties(${_trgt} PROPERTIES
			MAP_IMPORTED_CONFIG_DEBUG ${_tconfig} )
	    endif()
	    set_target_properties(${_trgt} PROPERTIES
		    MAP_IMPORTED_CONFIG_MINSIZEREL ${_tconfig}
		    MAP_IMPORTED_CONFIG_${_rconfigs} ${_tconfig} )
	    break()
	endif()
    endforeach()
endfunction(od_map_configurations)

function( get_install_dir _trgt _var prepend )
    set( _instdir )
    if ( ${_trgt} STREQUAL "OpenSSL::SSL" OR ${_trgt} STREQUAL "OpenSSL::Crypto" )
	set( _instdir "OpenSSL" )
    elseif ( ${_trgt} STREQUAL "Fontconfig::Fontconfig" )
	set( _instdir "fontconfig" )
    elseif ( ${_trgt} STREQUAL "Freetype::Freetype" )
	set( _instdir "freetype" )
    elseif ( ${_trgt} STREQUAL "PNG::PNG" )
	set( _instdir "png" )
    elseif ( ${_trgt} STREQUAL "ZLIB::ZLIB" )
	set( _instdir "zlib" )
    endif()
    if ( NOT "${_instdir}" STREQUAL "" )
	if ( prepend )
	    set( _instdir "/${_instdir}" )
	else()
	    set( _instdir "${_instdir}/" )
	endif()
    endif()
    set( ${_var} "${_instdir}" PARENT_SCOPE )
endfunction(get_install_dir)

macro( OD_INSTALL_LIBRARY LIBNM INSTDEST )
    if ( inst_config )
	install( PROGRAMS ${LIBNM} DESTINATION "${INSTDEST}"
		 CONFIGURATIONS ${inst_config} )
    else()
	install( PROGRAMS ${LIBNM} DESTINATION "${INSTDEST}" )
    endif()
    get_filename_component( _FILENAME "${LIBNM}" NAME )
    if ( IS_SYMLINK "${LIBNM}" )
	get_filename_component( PROGFLOC "${LIBNM}" REALPATH )
	if ( inst_config )
	    install( PROGRAMS "${PROGFLOC}" DESTINATION "${INSTDEST}"
		     CONFIGURATIONS ${inst_config} )
	else()
	    install( PROGRAMS "${PROGFLOC}" DESTINATION "${INSTDEST}" )
	endif()
	unset( PROGFLOC )
    endif()
    unset( _FILENAME )
    unset( _INST_DIR )
endmacro(OD_INSTALL_LIBRARY)

function( od_get_imported_link_dependent_libs trgt _var )

    set( _libs )

    get_target_property( TARGET_IMPORTED_LINK_DEP_LIBS ${trgt}
			 IMPORTED_LINK_DEPENDENT_LIBRARIES )
    if ( TARGET_IMPORTED_LINK_DEP_LIBS )
	foreach( IMPORTED_LINK_DEP_LIB ${TARGET_IMPORTED_LINK_DEP_LIBS} )
	    list( APPEND _libs "${IMPORTED_LINK_DEP_LIB}" )
	endforeach()
	unset( TARGET_IMPORTED_LINK_DEP_LIBS )
    endif()
    get_target_property( TARGET_CONFIGS ${trgt} IMPORTED_CONFIGURATIONS )
    foreach( config ${TARGET_CONFIGS} )
	get_target_property( TARGET_IMPORTED_LINK_DEP_LIBS_${config} ${trgt} IMPORTED_LINK_DEPENDENT_LIBRARIES_${config} )
	if( NOT TARGET_IMPORTED_LINK_DEP_LIBS_${config} )
	    continue()
	endif()
	foreach( TARGET_IMPORTED_LINK_DEP_LIB_${config} ${TARGET_IMPORTED_LINK_DEP_LIBS_${config}} )
	    list( APPEND _libs "${TARGET_IMPORTED_LINK_DEP_LIB_${config}}" )
	endforeach()
	unset( TARGET_IMPORTED_LINK_DEP_LIBS_${config} )
    endforeach()
    unset( TARGET_CONFIGS )

    list( REMOVE_DUPLICATES _libs )
    if ( UNIX )
	if ( APPLE )
	    list( REMOVE_ITEM _libs Qt5::Gui_OpenGL Qt5::Gui_AGL )
	else()
	    list( REMOVE_ITEM _libs Qt5::Gui_GL Qt5::Gui_EGL )
	endif()
    endif()
    set( ${_var} "${_libs}" PARENT_SCOPE )

endfunction( od_get_imported_link_dependent_libs )

function( od_get_imported_objects trgt _var _configs_out )

    set( _libs )
    set( _configs )
    get_target_property( TARGET_IMPORTED_OBJECTS ${trgt} IMPORTED_OBJECTS )
    if ( TARGET_IMPORTED_OBJECTS )
	foreach( IMPORTED_OBJECT ${TARGET_IMPORTED_OBJECTS} )
	    list( APPEND _libs "${IMPORTED_OBJECT}" )
	    list( APPEND _configs "All" )
	endforeach()
	unset( TARGET_IMPORTED_OBJECTS )
    endif()
    get_target_property( TARGET_CONFIGS ${trgt} IMPORTED_CONFIGURATIONS )
    foreach( config ${TARGET_CONFIGS} )
	get_target_property( TARGET_IMPORTED_OBJECTS_${config} ${trgt} IMPORTED_OBJECTS_${config} )
	if ( NOT TARGET_IMPORTED_OBJECTS_${config} )
	    continue()
	endif()
	set( inst_config ${config} )
	if ( "RELEASE" IN_LIST config AND NOT "MINSIZEREL" IN_LIST config )
	    list( APPEND inst_config MINSIZEREL )
	endif()
	if ( "RELEASE" IN_LIST config AND NOT "RELWITHDEBINFO" IN_LIST config )
	    list( APPEND inst_config RELWITHDEBINFO )
	endif()
	foreach( TARGET_IMPORTED_OBJECT_${config} ${TARGET_IMPORTED_OBJECTS_${config}} )
	    list( APPEND _libs "${TARGET_IMPORTED_OBJECT_${config}}" )
	    list( APPEND _configs ${config} )
	endforeach()
	unset( inst_config )
	unset( TARGET_IMPORTED_OBJECTS_${config} )
    endforeach()
    unset( TARGET_CONFIGS )

    set( ${_var} "${_libs}" PARENT_SCOPE )
    set( ${_configs_out} "${_configs}" PARENT_SCOPE )

endfunction( od_get_imported_objects trgt )

macro( OD_INSTALL_IMPORTED_LINK_DEPS TRGT )

    od_get_imported_link_dependent_libs( ${TRGT} TRGT_IMPORTED_LINK_DEPS )
    foreach( _imported_trgt ${TRGT_IMPORTED_LINK_DEPS} )
	if ( TARGET ${_imported_trgt} )
	    get_target_property( TARGET_TYPE ${_imported_trgt} TYPE )
	    if ( ${TARGET_TYPE} STREQUAL "SHARED_LIBRARY" )
		set( _INSTDIR )
		if ( UNIX AND NOT APPLE )
		    get_install_dir( ${_imported_trgt} _INSTDIR ON )
		endif()
		install( IMPORTED_RUNTIME_ARTIFACTS ${_imported_trgt}
			 LIBRARY DESTINATION "${OD_LIBRARY_DIRECTORY}${_INSTDIR}"
			 FRAMEWORK DESTINATION "${OD_LIBRARY_DIRECTORY}"
			 RUNTIME DESTINATION "${OD_LOCATION_DIRECTORY}" )
		unset( _INSTDIR )
	    endif()
	    unset( TARGET_TYPE )
	endif()
    endforeach()
    unset( TRGT_IMPORTED_LINK_DEPS )

endmacro( OD_INSTALL_IMPORTED_LINK_DEPS )

macro( OD_INSTALL_IMPORTED_OBJECTS TRGT )

    od_get_imported_objects( ${TRGT} TRGT_IMPORTED_OBJECTS IMPORTED_CONFIGS )
    foreach( _imported_obj _inst_config IN ZIP_LISTS TRGT_IMPORTED_OBJECTS IMPORTED_CONFIGS )
	if ( NOT "${_inst_config}" STREQUAL "All" )
	    set( inst_config ${_inst_config} )
	endif()
	OD_INSTALL_LIBRARY( "${_imported_obj}" "${OD_LOCATION_DIRECTORY}" )
	unset( inst_config )
    endforeach()
    unset( TRGT_IMPORTED_OBJECTS )
    unset( IMPORTED_CONFIGS )

endmacro( OD_INSTALL_IMPORTED_OBJECTS TRGT )

macro( OD_INSTALL_DEPENDENCIES MOD_DEPS )
    foreach( TRGT ${MOD_DEPS} )
	get_target_property( TARGET_TYPE ${TRGT} TYPE )
	if ( ${TARGET_TYPE} STREQUAL "SHARED_LIBRARY" )
	    set( _INSTDIR )
	    if ( UNIX AND NOT APPLE )
		get_install_dir( ${TRGT} _INSTDIR ON )
	    endif()
	    install( IMPORTED_RUNTIME_ARTIFACTS ${TRGT}
		     LIBRARY DESTINATION "${OD_LIBRARY_DIRECTORY}${_INSTDIR}"
		     FRAMEWORK DESTINATION "${OD_LIBRARY_DIRECTORY}"
		     RUNTIME DESTINATION "${OD_LOCATION_DIRECTORY}" )
	    unset( _INSTDIR )
	    OD_INSTALL_IMPORTED_LINK_DEPS( ${TRGT} )
	    OD_INSTALL_IMPORTED_OBJECTS( ${TRGT} )
	elseif ( UNIX AND ${TARGET_TYPE} STREQUAL "INTERFACE_LIBRARY" )
	    install( PROGRAMS "$<TARGET_PROPERTY:${TRGT},IMPORTED_LOCATION_RELEASE>"
		     DESTINATION "${OD_LOCATION_DIRECTORY}" )
	endif()
	unset( TARGET_TYPE )
    endforeach()

endmacro(OD_INSTALL_DEPENDENCIES)

function( od_get_library_filename LIBNM OD_LIBFNM )
    if( WIN32 )
	set( LIBFNM "${LIBNM}" )
    else()
	set( LIBFNM "lib${LIBNM}" )
    endif()
    set( LIBFNM "${OD_LOCATION_DIRECTORY}/${LIBFNM}$<$<CONFIG:Debug>:${CMAKE_DEBUG_POSTFIX}>.${SHLIB_EXTENSION}" )
    set( ${OD_LIBFNM} ${LIBFNM} PARENT_SCOPE ) 
endfunction()

function( od_get_application_filename APPNM OD_APPNM )
    set( EXECFNM "${OD_RUNTIME_DIRECTORY}/${APPNM}$<$<CONFIG:Debug>:${CMAKE_DEBUG_POSTFIX}>" )
    if( WIN32 )
	set( EXECFNM "${EXECFNM}.exe" )
    endif()
    set( ${OD_APPNM} "${EXECFNM}" PARENT_SCOPE )
endfunction()

function( od_find_library _var)
    foreach( _lib ${ARGN} )
	if ( DEFINED LIBSEARCHPATHS )
	    find_library( ODLIBLOC ${_lib} PATHS ${LIBSEARCHPATHS} NO_DEFAULT_PATH )
	    if ( NOT ODLIBLOC )
		find_library( ODLIBLOC ${_lib} PATHS ${LIBSEARCHPATHS} )
	    endif()
	else()
	    find_library( ODLIBLOC ${_lib} )
	endif()
	if ( ODLIBLOC )
	    break()
	endif()
    endforeach()
    if ( ODLIBLOC )
	set(${_var} "${ODLIBLOC}" PARENT_SCOPE)
    endif()
    unset( ODLIBLOC CACHE )
endfunction()

OPTION ( OD_CREATE_COMPILE_DATABASE "Create compile_commands.json database for analyser tools to use" OFF )
if ( OD_CREATE_COMPILE_DATABASE )
    set( CMAKE_EXPORT_COMPILE_COMMANDS "ON" )
endif()

macro( GET_OD_BASE_EXECUTABLES )
    foreach(  MODULE ${OD_CORE_MODULE_NAMES_od} ${OD_SPECPROGS} ${OD_PLUGINS} )
	list( APPEND OD_BASE_EXECUTABLE ${OD_${MODULE}_PROGS} )
    endforeach()
endmacro()


function( add_fontconfig _var )

    set( _libs )

    if ( TARGET Fontconfig::Fontconfig )
	list ( APPEND _libs Fontconfig::Fontconfig )
    else()
	message( FATAL_ERROR "Required system library not found: libfontconfig" )
    endif()

    if ( TARGET Freetype::Freetype )
	list ( APPEND _libs Freetype::Freetype )
    else()
	message( FATAL_ERROR "Required system library not found: libfreetype" )
    endif()

    if ( TARGET PNG::PNG )
	list ( APPEND _libs PNG::PNG )
	if ( TARGET ZLIB::ZLIB )
	    list ( APPEND _libs ZLIB::ZLIB )
	endif()
    else()
	message( FATAL_ERROR "Required system library not found: libpng" )
    endif()

    set( ${_var} "${_libs}" PARENT_SCOPE )
endfunction(add_fontconfig)

function( get_thirdparty_targets _var )
    set( _rawdeps )

    foreach( mod IN LISTS ARGN )
	if ( OD_${mod}_EXTERNAL_LIBS )
	    foreach( trgt ${OD_${mod}_EXTERNAL_LIBS} )
		if ( TARGET ${trgt} )
		    list( APPEND _rawdeps ${trgt} )
		endif()
	    endforeach()
	endif()
	if ( OD_${mod}_EXTERNAL_RUNTIME_LIBS )
	    foreach( trgt ${OD_${mod}_EXTERNAL_RUNTIME_LIBS} )
		if ( TARGET ${trgt} )
		    list( APPEND _rawdeps ${trgt} )
		endif()
	    endforeach()
	endif()
	list( REMOVE_DUPLICATES _rawdeps )
    endforeach()

    set( _deps )
    set( _allrawdeps ${_rawdeps} )
    foreach( _dep ${_allrawdeps} )
	od_get_imported_link_dependent_libs( ${_dep} dep_imported_link_dependent_libs )
	list( APPEND _allrawdeps ${dep_imported_link_dependent_libs} )
    endforeach()
    list( REMOVE_DUPLICATES _allrawdeps )
    foreach( _dep ${_allrawdeps} )
	get_target_property( TARGET_TYPE ${_dep} TYPE )
	if ( TARGET_TYPE )
	    if ( "${TARGET_TYPE}" STREQUAL "SHARED_LIBRARY" )
		list( APPEND _deps ${_dep} )
	    elseif ( UNIX AND "${TARGET_TYPE}" STREQUAL "INTERFACE_LIBRARY" )
		list( APPEND _deps ${_dep} )
	    endif()
	endif()
    endforeach()
    list( REMOVE_DUPLICATES _deps )
    set( ${_var} "${_deps}" PARENT_SCOPE )
endfunction(get_thirdparty_targets)

function( get_framework_name _trgt _var )
    set( framework_dir )
    get_target_property( IMPORT_FRAMEWORK ${_dep} FRAMEWORK )
    if ( NOT IMPORT_FRAMEWORK )
	return()
    endif()
    get_target_property( IMPORT_LOCATION ${_dep} LOCATION )
    if ( NOT IMPORT_LOCATION OR NOT EXISTS "${IMPORT_LOCATION}" )
	return()
    endif()
    set( framework_dir "${IMPORT_LOCATION}" )
    while( NOT "${framework_dir}" STREQUAL "" )
	get_filename_component( dirnm "${framework_dir}" NAME )
	if ( dirnm MATCHES ".framework$" )
	    set( framework_dir "${dirnm}" )
	    break()
	endif()
	get_filename_component( framework_dir "${framework_dir}" DIRECTORY )
    endwhile()
    set( ${_var} "${framework_dir}" PARENT_SCOPE )
endfunction(get_framework_name)

function( get_thirdparty_libs _deps _var )
    set( _libs )
    set( _alldeps ${_deps} )
    foreach( _dep ${_deps} )
	od_get_imported_link_dependent_libs( ${_dep} dep_imported_link_dependent_libs )
	list( APPEND _alldeps ${dep_imported_link_dependent_libs} )
    endforeach()
    list( REMOVE_DUPLICATES _alldeps )
    foreach( _dep ${_alldeps} )
	if ( WIN32 )
	    list( APPEND _libs "$<TARGET_FILE_NAME:${_dep}>" )
	else()
	    if ( APPLE )
		set( _lib )
		get_framework_name( ${_dep} _lib )
	    endif()
	    if ( _lib )
		list( APPEND _libs "${_lib}" )
		unset( _lib )
	    else()
		get_target_property( TARGET_TYPE ${_dep} TYPE )
		if ( TARGET_TYPE )
		    if ( "${TARGET_TYPE}" STREQUAL "SHARED_LIBRARY" )
			set( _INSTDIR )
			if ( NOT APPLE )
			    get_install_dir( ${_dep} _INSTDIR OFF )
			endif()
			list( APPEND _libs "${_INSTDIR}$<TARGET_SONAME_FILE_NAME:${_dep}>" )
			unset( _INSTDIR )
		    elseif ( UNIX AND "${TARGET_TYPE}" STREQUAL "INTERFACE_LIBRARY" )
			get_filename_component( _libfile "$<TARGET_PROPERTY:${_dep},IMPORTED_SONAME_RELEASE>" NAME )
			list( APPEND _libs "${_libfile}" )
			unset( _libfile )
		    endif()
		endif()
		unset( TARGET_TYPE )
	    endif()
	endif()
	od_get_imported_objects( ${_dep} dep_imported_objects imported_configs )
	if ( NOT "${dep_imported_objects}" STREQUAL "" )
	    foreach( _imported_obj _inst_config IN ZIP_LISTS dep_imported_objects imported_configs )
		get_filename_component( _imported_fnm ${_imported_obj} NAME )
		if ( "${_inst_config}" STREQUAL "All" )
		    list( APPEND _libs "${_imported_fnm}" )
		else()
		    list( APPEND _libs "$<$<CONFIG:${_inst_config}>:${_imported_fnm}>" )
		endif()
	    endforeach()
	endif()
    endforeach()

    if ( WIN32 )
	set( CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE )
	include( InstallRequiredSystemLibraries )
	foreach( _lib ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} )
	    get_filename_component( MSVCDLL "${_lib}" NAME )
	    list( APPEND _libs ${MSVCDLL} )
	endforeach()
    elseif ( UNIX AND NOT APPLE )
	list( APPEND _libs systemlibs/libstdc++.so.6 systemlibs/libgcc_s.so.1 )
    endif()

    set( ${_var} "${_libs}" PARENT_SCOPE )
endfunction(get_thirdparty_libs)

function (get_template_filepath template_name template_filepath)
	if ( EXISTS "${OpendTect_DIR}/CMakeModules/templates/${template_name}.in" )
		set( _template_filepath "${OpendTect_DIR}/CMakeModules/templates/${template_name}.in" )
	else()
		if ( EXISTS "${OD_BINARY_BASEDIR}/CMakeModules/templates/${template_name}.in" )
			set( _template_filepath "${OD_BINARY_BASEDIR}/CMakeModules/templates/${template_name}.in" )
		elseif ( EXISTS "${OD_BINARY_BASEDIR}/Contents/Resources/CMakeModules/templates/${template_name}.in" )
			set( _template_filepath "${OD_BINARY_BASEDIR}/Contents/Resources/CMakeModules/templates/${template_name}.in" )
		else()
			message( FATAL_ERROR "Template file ${template_name} not found" )
		endif()
	endif()
    set( ${template_filepath} "${_template_filepath}" PARENT_SCOPE )
endfunction(get_template_filepath)


macro( testprops tgt )
    set(props
        DEBUG_POSTFIX
	DEBUG_OUTPUT_NAME
	MINSIZEREL_OUTPUT_NAME
	RELWITHDEBINFO_OUTPUT_NAME
        RELEASE_OUTPUT_NAME
        LABELS
        COMPILE_FLAGS
        LINK_FLAGS
        VERSION
        SOVERSION
	ARCHIVE_OUTPUT_DIRECTORY
        ARCHIVE_OUTPUT_DIRECTORY_DEBUG
	ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL
	ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO
        ARCHIVE_OUTPUT_DIRECTORY_RELEASE
        LIBRARY_OUTPUT_DIRECTORY
	LIBRARY_OUTPUT_DIRECTORY_DEBUG
	LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL
	LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO
	LIBRARY_OUTPUT_DIRECTORY_RELEASE
        RUNTIME_OUTPUT_DIRECTORY
	RUNTIME_OUTPUT_DIRECTORY_DEBUG
	RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL
	RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO
	RUNTIME_OUTPUT_DIRECTORY_RELEASE
	FRAMEWORK
	FRAMEWORK_VERSION
        LOCATION
        LOCATION_DEBUG
	LOCATION_MINSIZEREL
	LOCATION_RELWITHDEBINFO
        LOCATION_RELEASE
        IMPORT_CHECK_TARGETS
        IMPORT_CHECK_FILES_FOR_${tgt}
	IMPORTED
	IMPORTED_COMMON_LANGUAGE_RUNTIME
        IMPORTED_CONFIGURATIONS
	IMPORTED_GLOBAL
        IMPORTED_IMPLIB
        IMPORTED_IMPLIB_DEBUG
	IMPORTED_IMPLIB_MINSIZEREL
	IMPORTED_IMPLIB_RELWITHDEBINFO
        IMPORTED_IMPLIB_RELEASE
        IMPORTED_LIBNAME
        IMPORTED_LIBNAME_DEBUG
	IMPORTED_LIBNAME_MINSIZEREL
	IMPORTED_LIBNAME_RELWITHDEBINFO
        IMPORTED_LIBNAME_RELEASE
        IMPORTED_LINK_DEPENDENT_LIBRARIES
        IMPORTED_LINK_DEPENDENT_LIBRARIES_DEBUG
	IMPORTED_LINK_DEPENDENT_LIBRARIES_MINSIZEREL
	IMPORTED_LINK_DEPENDENT_LIBRARIES_RELWITHDEBINFO
        IMPORTED_LINK_DEPENDENT_LIBRARIES_RELEASE
	IMPORTED_LINK_INTERFACE_LANGUAGES
	IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG
	IMPORTED_LINK_INTERFACE_LANGUAGES_MINSIZEREL
	IMPORTED_LINK_INTERFACE_LANGUAGES_RELWITHDEBINFO
        IMPORTED_LINK_INTERFACE_LIBRARIES
        IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG
	IMPORTED_LINK_INTERFACE_LIBRARIES_MINSIZEREL
	IMPORTED_LINK_INTERFACE_LIBRARIES_RELWITHDEBINFO
	IMPORTED_LINK_INTERFACE_MULTIPLICITY
	IMPORTED_LINK_INTERFACE_MULTIPLICITY_DEBUG
	IMPORTED_LINK_INTERFACE_MULTIPLICITY_MINSIZEREL
	IMPORTED_LINK_INTERFACE_MULTIPLICITY_RELWITHDEBINFO
	IMPORTED_LINK_INTERFACE_MULTIPLICITY_RELEASE
	IMPORTED_LOCATION
	IMPORTED_LOCATION_DEBUG
	IMPORTED_LOCATION_MINSIZEREL
	IMPORTED_LOCATION_RELWITHDEBINFO
	IMPORTED_LOCATION_RELEASE
	IMPORTED_NO_SONAME
	IMPORTED_NO_SONAME_DEBUG
	IMPORTED_NO_SONAME_MINSIZEREL
	IMPORTED_NO_SONAME_RELWITHDEBINFO
	IMPORTED_NO_SONAME_RELEASE
	IMPORTED_NO_SYSTEM
	IMPORTED_OBJECTS
	IMPORTED_OBJECTS_DEBUG
	IMPORTED_OBJECTS_MINSIZEREL
	IMPORTED_OBJECTS_RELWITHDEBINFO
	IMPORTED_OBJECTS_RELEASE
        IMPORTED_SONAME
        IMPORTED_SONAME_DEBUG
	IMPORTED_SONAME_MINSIZEREL
	IMPORTED_SONAME_RELWITHDEBINFO
        IMPORTED_SONAME_RELEASE
	IMPORTED_PREFIX
	IMPORTED_SUFFIX
        IMPORTED_TARGETS
        INTERFACE_INCLUDE_DIRECTORIES
	MAP_IMPORTED_CONFIG_DEBUG
	MAP_IMPORTED_CONFIG_MINSIZEREL
	MAP_IMPORTED_CONFIG_RELWITHDEBINFO
	MAP_IMPORTED_CONFIG_RELEASE
	NAME
	NO_SONAME
	OUTPUT_NAME
	OUTPUT_NAME_DEBUG
	OUTPUT_NAME_MINSIZEREL
	OUTPUT_NAME_RELWITHDEBINFO
	OUTPUT_NAME_RELEASE
	TYPE
    )

    foreach(p ${props})
        get_property(v TARGET ${tgt} PROPERTY ${p})
        get_property(d TARGET ${tgt} PROPERTY ${p} DEFINED)
        get_property(s TARGET ${tgt} PROPERTY ${p} SET)
        if( s )
            message( STATUS "tgt='${tgt}' p='${p}'" )
            message( STATUS "  value='${v}'" )
            message( STATUS "  defined='${d}'" )
            message( STATUS "  set='${s}'" )
            message( STATUS "" )
        endif()
    endforeach()
endmacro()


# Used compile_commands.json for include-what-you-use
# python3 /usr/local/bin/iwyu_tool.py -p . > iwyu_results.txt
# Note that this tool is of limited use as it wants to dictate all includes
