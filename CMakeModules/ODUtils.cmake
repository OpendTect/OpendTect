#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

if ( (CMAKE_GENERATOR STREQUAL "Unix Makefiles") OR
     (CMAKE_GENERATOR STREQUAL "Ninja") OR
     (CMAKE_GENERATOR STREQUAL "NMake Makefiles") )
    if ( CMAKE_BUILD_TYPE STREQUAL "" )
	set ( DEBUGENV $ENV{DEBUG} )
	if ( DEBUGENV AND
	    ( ( DEBUGENV MATCHES "yes" ) OR
	      ( DEBUGENV MATCHES "Yes" ) OR
	      ( DEBUGENV MATCHES "YES" ) ) )
	    set ( CMAKE_BUILD_TYPE "Debug"
		  CACHE STRING "Debug or Release" FORCE )
	else()
	    set ( CMAKE_BUILD_TYPE "Release"
		  CACHE STRING "Debug or Release" FORCE)
	endif()

	message( STATUS "Setting CMAKE_BUILD_TYPE to ${CMAKE_BUILD_TYPE}" )
    endif()

    set( OD_BUILDSUBDIR "/${CMAKE_BUILD_TYPE}" )
endif()

add_definitions("-D__cmake__")

if ( APPLE )
    set( MISC_INSTALL_PREFIX Contents/Resources )
else()
    set( MISC_INSTALL_PREFIX . )
endif()


set ( OD_SOURCELIST_FILE ${CMAKE_BINARY_DIR}/CMakeModules/sourcefiles_${OD_SUBSYSTEM}.txt )
file ( REMOVE ${OD_SOURCELIST_FILE} )

if ( APPLE )
    set ( OD_EXEC_OUTPUT_RELPATH "Contents/MacOS" )
    set ( OD_EXEC_RELPATH_RELEASE ${OD_EXEC_OUTPUT_RELPATH} )
    set ( OD_EXEC_RELPATH_DEBUG ${OD_EXEC_OUTPUT_RELPATH} )
    set ( OD_LIB_OUTPUT_RELPATH "Contents/Frameworks" )
    set ( OD_LIB_RELPATH_RELEASE ${OD_LIB_OUTPUT_RELPATH} )
    set ( OD_LIB_RELPATH_DEBUG ${OD_LIB_OUTPUT_RELPATH} )
else()
    set ( OD_EXEC_OUTPUT_RELPATH "bin/${OD_PLFSUBDIR}${OD_BUILDSUBDIR}" )
    set ( OD_EXEC_RELPATH_RELEASE "bin/${OD_PLFSUBDIR}/Release" )
    set ( OD_EXEC_RELPATH_DEBUG "bin/${OD_PLFSUBDIR}/Debug" )
    set ( OD_LIB_OUTPUT_RELPATH "bin/${OD_PLFSUBDIR}${OD_BUILDSUBDIR}" )
    set ( OD_LIB_RELPATH_RELEASE "bin/${OD_PLFSUBDIR}/Release" )
    set ( OD_LIB_RELPATH_DEBUG "bin/${OD_PLFSUBDIR}/Debug" )
endif()

set ( OD_EXEC_OUTPUT_PATH "${CMAKE_BINARY_DIR}/${OD_EXEC_OUTPUT_RELPATH}" )
set ( OD_LIB_OUTPUT_PATH "${CMAKE_BINARY_DIR}/${OD_LIB_OUTPUT_RELPATH}" )
set ( OD_DATA_INSTALL_RELPATH "${MISC_INSTALL_PREFIX}/data" )

set ( OD_EXEC_INSTALL_PATH_RELEASE ${OD_EXEC_RELPATH_RELEASE} )
set ( OD_EXEC_INSTALL_PATH_DEBUG ${OD_EXEC_RELPATH_DEBUG} )

set ( OD_LIB_INSTALL_PATH_RELEASE ${OD_LIB_RELPATH_RELEASE} )
set ( OD_LIB_INSTALL_PATH_DEBUG ${OD_LIB_RELPATH_DEBUG} )

set ( CMAKE_PDB_OUTPUT_DIRECTORY "${OD_EXEC_OUTPUT_PATH}" )

set ( OD_BUILD_VERSION "${OpendTect_VERSION_MAJOR}.${OpendTect_VERSION_MINOR}.${OpendTect_VERSION_PATCH}")
set ( OD_API_VERSION "${OpendTect_VERSION_MAJOR}.${OpendTect_VERSION_MINOR}.${OpendTect_VERSION_PATCH}" )

set ( OD_MAIN_EXEC od_main )
if ( WIN32 )
    list( APPEND OD_MAIN_EXEC od_main_console )
endif()
set ( OD_ATTRIB_EXECS od_process_attrib od_process_attrib_em od_stratamp )
set ( OD_VOLUME_EXECS od_process_volume )
set ( OD_SEIS_EXECS od_cbvs_browse od_copy_seis od_process_2dto3d od_process_segyio )
set ( OD_PRESTACK_EXECS od_process_prestack )
set ( OD_ZAXISTRANSFORM_EXECS od_process_time2depth )

#Should not be here.
set ( DGB_SR_EXECS od_SynthRock )
set ( DGB_ML_EXECS od_deeplearn_apply )
set ( DGB_SEGY_EXECS od_DeepLearning_CC )
set ( DGB_ML_UIEXECS od_DeepLearning od_DeepLearning_CC od_DeepLearning_EM od_DeepLearning_TM od_DeepLearning_ModelImport )
set ( DGB_PRO_EXECS od_petrelhortransfer od_petrelseistransfer od_fullpetrelsurveyimp )
set ( DGB_PRO_UIEXECS od_Basemap od_LogPlot od_LogViewer )

option ( OD_DISABLE_EXTERNAL_LIBS_CHECK "Disabling automatic retrieval of external dependency for plugins" ON )
mark_as_advanced( FORCE OD_DISABLE_EXTERNAL_LIBS_CHECK )

macro( OD_ADD_EXTERNALS )
    set( CMAKE_EXT_CXX_FLAGS "${CMAKE_CXX_FLAGS}" )
    set( EXTPLFTOOLSET "" )
    if ( CMAKE_GENERATOR MATCHES "Ninja" )
	if ( UNIX AND EXISTS "${CMAKE_MAKE_PROGRAM}" )
	    set( EXTPLFARCH "-DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}" )
	endif()
	set( EXTGENERATOR "-G${CMAKE_GENERATOR}" )
    elseif ( WIN32 AND CMAKE_GENERATOR MATCHES "Visual Studio" )
	set( EXTGENERATOR "-G ${CMAKE_GENERATOR}" )
	if ( NOT "${CMAKE_VS_PLATFORM_NAME_DEFAULT}" STREQUAL "" )
	    set( EXTPLFARCH "-A ${CMAKE_VS_PLATFORM_NAME_DEFAULT}" )
	endif()
	set( EXTPLFTOOLSET "-T v${MSVC_TOOLSET_VERSION}" )
	string( REPLACE "/MP " "" CMAKE_EXT_CXX_FLAGS ${CMAKE_EXT_CXX_FLAGS} ) 
    else()
	find_program( NINJA_BIN "ninja" )
	if ( NINJA_BIN )
	    set( EXTGENERATOR "-GNinja" )
	endif()
    endif()
    OD_ADD_BREAKPAD()
    OD_ADD_QT()
    OD_ADD_PROJ()
    OD_ADD_OSG()
    OD_ADD_ZLIB()
    OD_FIND_HDF5()
    OD_FIND_OPENSSL()
    unset( NINJA_BIN CACHE )
endmacro(OD_ADD_EXTERNALS)

#Macro for going through a list of modules and adding them
macro ( OD_ADD_MODULES )
    set( DIR ${ARGV0} )
    if ( "${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}" )
	set( MODULE_OUTPUT_DIR ${DIR} )
    else()
	string(REPLACE ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} MODULE_OUTPUT_DIR ${DIR})
    endif()
    string(TIMESTAMP YEAR %Y)
    foreach( OD_MODULE_NAME ${ARGV} )
	if ( NOT ${OD_MODULE_NAME} STREQUAL ${DIR} )
	    add_subdirectory( ${DIR}/${OD_MODULE_NAME} 
			      ${MODULE_OUTPUT_DIR}/${OD_MODULE_NAME} )
	endif()
    endforeach()
    unset( MODULE_OUTPUT_DIR )
endmacro(OD_ADD_MODULES)

# Macro for going through a list of modules and adding them
# as optional targets
macro ( OD_ADD_OPTIONAL_MODULES )
    set( DIR ${ARGV0} )
    if ( "${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}" )
	set( MODULE_OUTPUT_DIR ${DIR} )
    else()
	string(REPLACE ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} MODULE_OUTPUT_DIR ${DIR})
    endif()
    foreach( OD_MODULE_NAME ${ARGV} )
	if ( NOT OD_MODULE_NAME MATCHES ${DIR} )
	    add_subdirectory( ${DIR}/${OD_MODULE_NAME} 
			      "${MODULE_OUTPUT_DIR}/${OD_MODULE_NAME}"
			      EXCLUDE_FROM_ALL )
	endif()
    endforeach()
    unset( MODULE_OUTPUT_DIR )
endmacro()


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
    # Start off with the link directories of the calling listfile's directory
    get_directory_property(_libdirs LINK_DIRECTORIES)

    # Add additional libraries passed to the function
    foreach(_lib ${ARGN})
	if ( EXISTS "${_lib}" )
	    set( _libdir "${_lib}" )
	else()
	    OD_READ_TARGETINFO( "${_lib}" )
	    set( _libdir "${SOURCEFILE}" )
	endif()
	if ( EXISTS "${_libdir}" )
	    get_filename_component( _libdir "${_libdir}" DIRECTORY )
	    list(APPEND _libdirs "${_libdir}")
	endif()
    endforeach()
    clean_directory_list(_libdirs)

    if ( UNIX )
	set(${_var} "${_libdirs}" PARENT_SCOPE)
    else()
	# Now, build a list of potential dll directories
	set(_dlldirs)
	foreach(_libdir ${_libdirs})
	    get_filename_component(_dlldir "${_libdir}/../bin" ABSOLUTE)
	    if ( EXISTS "${_dlldir}" )
		list(APPEND _dlldirs "${_dlldir}")
	    else()
		list(APPEND _dlldirs "${_libdir}")
	    endif()
	endforeach()

	clean_directory_list(_dlldirs)
	set(${_var} "${_dlldirs}" PARENT_SCOPE)
    endif()
endfunction()

function(guess_extruntime_library_dirs _var)
    foreach(_lib ${ARGN})
	if ( EXISTS "${_lib}" )
	    set( LIBLOC ${_lib} )
	else()
	    get_target_property( LIBTYPE ${_lib} TYPE )
	    if ( NOT "${LIBTYPE}" STREQUAL "INTERFACE_LIBRARY" )
		get_target_property( LIBLOC ${_lib} IMPORTED_LOCATION )
	    endif()
	endif()
	if ( EXISTS "${LIBLOC}" )
	    get_filename_component( LIBDIR ${LIBLOC} DIRECTORY )
	    if ( WIN32 )
		get_filename_component( DLLDIR ${LIBDIR} DIRECTORY )
		set( DLLDIR "${DLLDIR}/bin" )
		if ( EXISTS ${DLLDIR} )
		    set( RUNTIMEDIR ${DLLDIR} )
		else()
		    set( RUNTIMEDIR ${LIBDIR} )
		endif()
	    else()
		set( RUNTIMEDIR ${LIBDIR} )
	    endif( WIN32 )
	    if ( EXISTS ${RUNTIMEDIR} )
		list( APPEND _libdirs ${RUNTIMEDIR} )
	    endif()
	endif()
    endforeach()
    clean_directory_list(_libdirs)
    set(${_var} "${_libdirs}" PARENT_SCOPE)
endfunction()

macro( OD_READ_TARGETINFO TARGETNM )
    get_target_property( LIB_BUILD_TYPE ${TARGETNM} IMPORTED_CONFIGURATIONS )
    list( LENGTH LIB_BUILD_TYPE LIB_NRBUILDS )
    if ( LIB_NRBUILDS GREATER 1 )
	if ( ${CMAKE_BUILD_TYPE} STREQUAL "Debug" AND "DEBUG" IN_LIST LIB_BUILD_TYPE )
	    set( LIB_BUILD_TYPE "DEBUG" )
	elseif ( ${CMAKE_BUILD_TYPE} STREQUAL "Release" AND "RELEASE" IN_LIST LIB_BUILD_TYPE )
	    set( LIB_BUILD_TYPE "RELEASE" )
	elseif( "RELEASE" IN_LIST LIB_BUILD_TYPE )
	    set( LIB_BUILD_TYPE "RELEASE" )
	elseif( "DEBUG" IN_LIST LIB_BUILD_TYPE )
	    set( LIB_BUILD_TYPE "DEBUG" )
	endif()
    endif()
    if ( LIB_BUILD_TYPE )
	set( IMPORTED_LOC_KEYWORD IMPORTED_LOCATION_${LIB_BUILD_TYPE} )
	set( IMPORTED_SONAME_KEYWORD IMPORTED_SONAME_${LIB_BUILD_TYPE} )
    else()
	set( IMPORTED_LOC_KEYWORD IMPORTED_LOCATION )
	set( IMPORTED_SONAME_KEYWORD IMPORTED_SONAME )
    endif()
    get_target_property( LIBTYPE ${TARGETNM} TYPE )
    if ( NOT "${LIBTYPE}" STREQUAL "INTERFACE_LIBRARY" )
	get_target_property( LIBLOC ${TARGETNM} ${IMPORTED_LOC_KEYWORD} )
    endif()
    if ( NOT LIBLOC )
	get_target_property( LIBLOC ${TARGETNM} LOCATION )
	if ( NOT EXISTS "${LIBLOC}" )
	    get_target_property( LIBLOC ${TARGETNM} LOCATION_Debug )
	    if ( EXISTS "${LIBLOC}" )
		set( LIB_BUILD_TYPE "Debug" )
	    else()
		get_target_property( LIBLOC ${TARGETNM} LOCATION_Release )
		if ( EXISTS "${LIBLOC}" )
		    set( LIB_BUILD_TYPE "Release" )
		endif()
	    endif()
	endif()
    endif()
    if ( UNIX AND NOT "${LIBTYPE}" STREQUAL "INTERFACE_LIBRARY" )
	get_target_property( LIBSONAME ${TARGETNM} ${IMPORTED_SONAME_KEYWORD} )
	if ( APPLE )
	   get_filename_component( LIBSONAME ${LIBSONAME} NAME )
	endif()
    endif()
    get_filename_component( LIBDIR ${LIBLOC} DIRECTORY )
    if ( LIBSONAME )
	set( LIBLOC "${LIBDIR}/${LIBSONAME}" )
    endif()

    if ( NOT LIBLOC )
	message( FATAL_ERROR "Unrecognized target to install: ${TARGETNM} ${LIBLOC}" )
    endif()

    set ( SOURCEFILE "${LIBLOC}" )
endmacro()

macro( OD_GET_WIN32_INST_VARS )
    get_filename_component( BASEFILENM ${FILEPATH} NAME_WE )
    get_filename_component( FILEEXT ${FILEPATH} EXT )
    if ( "${FILEEXT}" STREQUAL ".lib" )
	get_filename_component( DLLPATH ${FILEPATH} DIRECTORY )
	get_filename_component( DLLPATH ${DLLPATH} DIRECTORY )
	set( FILEPATH "" )
	set( DLLPATH "${DLLPATH}/bin" )
	if ( EXISTS "${DLLPATH}" )
	    file( GLOB DLLFILENMS "${DLLPATH}/*${BASEFILENM}.dll" )
	    file( GLOB DLLFILENMS2 "${DLLPATH}/*${BASEFILENM}*x64.dll" )
	    list( APPEND DLLFILENMS ${DLLFILENMS2} )
	    foreach( DLLFILE ${DLLFILENMS} )
		list( APPEND FILEPATH "${DLLFILE}" )
		get_filename_component( DLLBASEFILENM "${DLLFILE}" NAME_WE )
		file( GLOB PDBFILENMS "${DLLPATH}/${DLLBASEFILENM}.pdb" )
		foreach( PDBFILE ${PDBFILENMS} )
		    list( APPEND FILEPATH "${PDBFILE}" )
		endforeach()
	    endforeach()
	endif()
    elseif ( "${FILEEXT}" STREQUAL ".dll" OR "${FILEEXT}" STREQUAL ".DLL" )
	get_filename_component( DLLPATH ${FILEPATH} DIRECTORY )
	file( GLOB PDBFILENMS "${DLLPATH}/${BASEFILENM}*.pdb" )
	foreach( PDBFILE ${PDBFILENMS} )
	    list( APPEND FILEPATH "${PDBFILE}" )
	endforeach()
    endif()
    set( FILENAMES "" )
    foreach( FILENM ${FILEPATH} )
	get_filename_component( FILENAME ${FILENM} NAME )
	list( APPEND FILENAMES ${FILENAME} )
    endforeach()
endmacro()

macro( OD_GET_UNIX_INST_VARS )
    set( FILENAMES "" )
    foreach( FILENM ${FILEPATH} )
	get_filename_component( FILENAME ${FILENM} NAME )
	if ( APPLE )
	    set( LIBEXT ".dylib" )
	else()
	    set( LIBEXT ".so" )
	endif()
	get_filename_component( SRCFILENAME ${SOURCEFILE} NAME )
	get_filename_component( INSTPATH ${SOURCEFILE} DIRECTORY )
	if ( DEFINED LIBSONAME AND LIBSONAME )
	    set( FILENAME ${LIBSONAME} )
	    unset( LIBSONAME )
	else()
	    get_filename_component( FILEEXT ${SOURCEFILE} EXT )
	    if ( "${FILEEXT}" STREQUAL "${LIBEXT}" AND NOT "${FILENAME}" STREQUAL "${SRCFILENAME}" )
		if ( CMAKE_VERSION VERSION_GREATER_EQUAL 3.14 )
		    file(READ_SYMLINK "${SOURCEFILE}" FILENAME)
		else()
		    file(GLOB FILENAME "${SOURCEFILE}.???")
		    if ( NOT EXISTS ${FILENAME} )
			file(GLOB FILENAME "${SOURCEFILE}.??")
			if ( NOT EXISTS ${FILENAME} )
			    get_filename_component( FILENAME "${SOURCEFILE}" REALPATH )
			    get_filename_component( FILENAME "${FILENAME}" NAME )
			endif()
		    endif()
		endif()
		if ( EXISTS "${FILENAME}")
		    get_filename_component( FILENAME "${FILENAME}" NAME )
		endif()
	    else()
		set( FILENAME "${SRCFILENAME}" )
	    endif()
	endif()
	list( APPEND FILENAMES ${FILENAME} )
    endforeach()
endmacro( OD_GET_UNIX_INST_VARS )

macro( OD_GET_INSTALL_VARS SOURCE )
    if ( EXISTS "${SOURCE}" )
	set( SOURCEFILE "${SOURCE}" )
    else()
	OD_READ_TARGETINFO( ${SOURCE} )
    endif()
    get_filename_component( FILEPATH ${SOURCEFILE} REALPATH )
    if ( WIN32 )
	OD_GET_WIN32_INST_VARS()
    else()
	OD_GET_UNIX_INST_VARS()
    endif( WIN32 )
endmacro( OD_GET_INSTALL_VARS )

macro ( OD_INSTALL_SYSTEM_LIBRARY SOURCE )
    OD_GET_INSTALL_VARS( ${SOURCE} )
    list(LENGTH FILEPATH len)
    if ( len GREATER 0 )
	set( FONTLIBS fontconfig;freetype;png15;png16 )
	math(EXPR len "${len} - 1")
	foreach( val RANGE ${len} )
	    list(GET FILEPATH ${val} FILENM)
	    list(GET FILENAMES ${val} FILENAME)
	    if ( UNIX AND NOT APPLE )
		foreach( FONTLIB ${FONTLIBS} )
		    string(FIND ${FILENAME} ${FONTLIB} foundstring )
		    if ( NOT ${foundstring} EQUAL -1 )
			set( INST_SUFFIX "/${FONTLIB}" )
		    endif()
		endforeach()
	    endif()
#	    message( STATUS "Installing: ${FILENM} as ${FILENAME}" )
	    install( PROGRAMS ${FILENM} DESTINATION ${OD_LIB_INSTALL_PATH_DEBUG}${INST_SUFFIX}
		     RENAME ${FILENAME} CONFIGURATIONS Debug )
	    install( PROGRAMS ${FILENM} DESTINATION ${OD_LIB_INSTALL_PATH_RELEASE}${INST_SUFFIX}
		     RENAME ${FILENAME} CONFIGURATIONS Release )
	    OD_PREPARE_EXTERNALS_FILELIST( ${FILENAME} )
	    unset( INST_SUFFIX )
	endforeach()
    endif()
endmacro( OD_INSTALL_SYSTEM_LIBRARY )

macro ( OD_INSTALL_LIBRARY SOURCE )
    OD_INSTALL_SYSTEM_LIBRARY( "${SOURCE}" )
endmacro( OD_INSTALL_LIBRARY )

macro( OD_PREPARE_EXTERNALS_FILELIST SOURCE )
    get_filename_component( FILENAME "${SOURCE}" NAME )
    list( APPEND OD_THIRD_PARTY_FILES ${FILENAME} )
    list( REMOVE_DUPLICATES OD_THIRD_PARTY_FILES )
    get_directory_property(HASPARENT PARENT_DIRECTORY)
    if( HASPARENT )
	set ( OD_THIRD_PARTY_FILES ${OD_THIRD_PARTY_FILES} PARENT_SCOPE )
    endif()

endmacro(OD_PREPARE_EXTERNALS_FILELIST)

macro ( OD_INSTALL_RESSOURCE SOURCE )
    install( FILES "${SOURCE}" DESTINATION "${OD_LIB_INSTALL_PATH_DEBUG}"
	     CONFIGURATIONS Debug )
    install( FILES "${SOURCE}" DESTINATION "${OD_LIB_INSTALL_PATH_RELEASE}"
	     CONFIGURATIONS Release )

    OD_PREPARE_EXTERNALS_FILELIST( "${SOURCE}" )
endmacro( OD_INSTALL_RESSOURCE )

macro ( OD_INSTALL_PROGRAM SOURCE )
    install( PROGRAMS "${SOURCE}" DESTINATION "${OD_LIB_INSTALL_PATH_DEBUG}"
	     CONFIGURATIONS Debug )
    install( PROGRAMS "${SOURCE}" DESTINATION "${OD_LIB_INSTALL_PATH_RELEASE}"
	     CONFIGURATIONS Release )

    OD_PREPARE_EXTERNALS_FILELIST( "${SOURCE}" )
endmacro( OD_INSTALL_PROGRAM )

function( od_get_library_filename LIBNM OD_LIBFNM )
    if( WIN32 )
	set( LIBFNM "${LIBNM}${OD_STATIC_EXTENSION}" )
    elseif( APPLE )
	set( LIBFNM "${CMAKE_FIND_LIBRARY_PREFIXES}${LIBNM}${CMAKE_FIND_LIBRARY_SUFFIXES}" )
    else()
	set( LIBFNM "lib${LIBNM}.${SHLIB_EXTENSION}" )
    endif()
    set( ${OD_LIBFNM} ${LIBFNM} PARENT_SCOPE ) 
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


function( add_fontconfig OUTPUT_LIST INPUT_LIST )
    list( APPEND LIBLIST ${INPUT_LIST} )

    if ( APPLE )
	od_find_library( LIBFONTCONFIG libfontconfig.1.dylib )
    else()
	od_find_library( LIBFONTCONFIG libfontconfig.so.1 )
    endif()

    if ( LIBFONTCONFIG )
	list ( APPEND LIBLIST "${LIBFONTCONFIG}" )
    else()
	message( FATAL_ERROR "Required system library not found: libfontconfig" )
    endif()

    if ( APPLE )
	od_find_library( LIBFREETYPELOC libfreetype.6.dylib )
    else()
	od_find_library( LIBFREETYPELOC libfreetype.so.6 )
    endif()

    if ( LIBFREETYPELOC )
	list ( APPEND LIBLIST "${LIBFREETYPELOC}" )
    else()
	message( FATAL_ERROR "Required system library not found: libfreetype" )
    endif()

    if ( APPLE )
	od_find_library( LIBPNGLOC libpng16.16.dylib )
    else()
	od_find_library( LIBPNGLOC libpng16.so.16 libpng15.so.15 libpng12.so.0 )
    endif()

    if ( LIBPNGLOC )
	list ( APPEND LIBLIST "${LIBPNGLOC}" )
    else()
	message( FATAL_ERROR "Required system library not found: libpng" )
    endif()

    set( ${OUTPUT_LIST} ${LIBLIST} PARENT_SCOPE )
endfunction(add_fontconfig)

macro( testprops tgt )
    set(props
        DEBUG_OUTPUT_NAME
        DEBUG_POSTFIX
        RELEASE_OUTPUT_NAME
        LABELS
        COMPILE_FLAGS
        LINK_FLAGS
        VERSION
        SOVERSION
        ARCHIVE_OUTPUT_DIRECTORY_DEBUG
        LIBRARY_OUTPUT_DIRECTORY_DEBUG
        RUNTIME_OUTPUT_DIRECTORY_DEBUG
        ARCHIVE_OUTPUT_DIRECTORY_RELEASE
        LIBRARY_OUTPUT_DIRECTORY_RELEASE
        RUNTIME_OUTPUT_DIRECTORY_RELEASE
        ARCHIVE_OUTPUT_DIRECTORY
        LIBRARY_OUTPUT_DIRECTORY
        RUNTIME_OUTPUT_DIRECTORY
        LOCATION
        LOCATION_DEBUG
        LOCATION_RELEASE
        IMPORT_CHECK_TARGETS
        IMPORT_CHECK_FILES_FOR_${tgt}
        IMPORTED_CONFIGURATIONS
        IMPORTED_IMPLIB
        IMPORTED_IMPLIB_DEBUG
        IMPORTED_IMPLIB_RELEASE
        IMPORTED_LIBNAME
        IMPORTED_LIBNAME_DEBUG
        IMPORTED_LIBNAME_RELEASE
        IMPORTED_LOCATION
        IMPORTED_LOCATION_DEBUG
        IMPORTED_LOCATION_RELEASE
        IMPORTED_LINK_DEPENDENT_LIBRARIES
        IMPORTED_LINK_DEPENDENT_LIBRARIES_DEBUG
        IMPORTED_LINK_DEPENDENT_LIBRARIES_RELEASE
        IMPORTED_LINK_INTERFACE_LIBRARIES
        IMPORTED_LINK_INTERFACE_LIBRARIES_DEBUG
        IMPORTED_LINK_INTERFACE_LIBRARIES_RELEASE
        IMPORTED_SONAME
        IMPORTED_SONAME_DEBUG
        IMPORTED_SONAME_RELEASE
        IMPORTED_TARGETS
        INTERFACE_INCLUDE_DIRECTORIES
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
