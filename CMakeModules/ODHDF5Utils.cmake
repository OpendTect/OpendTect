#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

macro( OD_CLEANUP_HDF5 )
    unset( HDF5_DIFF_EXECUTABLE CACHE )
    unset( HDF5_DIR CACHE )
    unset( HDF5_IS_PARALLEL CACHE )
    unset( HDF5_USE_STATIC_LIBRARIES CACHE )
    list(APPEND COMPS C CXX )
    list(APPEND LIBSUFFIXES dl hdf5 hdf5_cpp m pthread z)
    foreach( COMP IN LISTS COMPS )
	unset( HDF5_${COMP}_COMPILER_EXECUTABLE CACHE )
	unset( HDF5_${COMP}_COMPILER_NO_INTERROGATE CACHE )
	unset( HDF5_${COMP}_INCLUDE_DIR CACHE )
	foreach( LIBSUFFIX IN LISTS LIBSUFFIXES )
	    unset( HDF5_${COMP}_LIBRARY_${LIBSUFFIX} CACHE )
	    unset( HDF5_${LIBSUFFIX}_LIBRARY_DEBUG CACHE )
	    unset( HDF5_${LIBSUFFIX}_LIBRARY_RELEASE CACHE )
	endforeach()
    endforeach()
    unset( HDF5_ROOT CACHE )
endmacro(OD_CLEANUP_HDF5)

macro( CHECKTARGETTYPE TARGETNAME )
    if(TARGET ${TARGETNAME})
	set( LIBISTARGET TRUE )
    else()
	set( LIBISTARGET FALSE )
    endif()
endmacro(CHECKTARGETTYPE)

macro( GETHDF5COMPDEF )
    CHECKTARGETTYPE(${HDF5_CXX_SHARED_LIBRARY})
    if ( LIBISTARGET )
	get_target_property( HDF5_COMPILEDEF ${HDF5_CXX_SHARED_LIBRARY} INTERFACE_COMPILE_DEFINITIONS )
    endif()
    if ( NOT DEFINED HDF5_COMPILEDEF OR NOT HDF5_COMPILEDEF )
	set( HDF5_COMPILEDEF "H5_BUILT_AS_DYNAMIC_LIB" )
    endif()
endmacro(GETHDF5COMPDEF)

macro( GET_HDF5_ROOT )
    if ( HDF5_DIFF_EXECUTABLE )
	get_filename_component( hdf5_path ${HDF5_DIFF_EXECUTABLE} DIRECTORY )
	get_filename_component( hdf5_path ${hdf5_path} DIRECTORY )
    elseif ( HDF5_INCLUDE_DIRS OR HDF5_INCLUDE_DIR OR HDF5_C_INCLUDE_DIR )
	if ( HDF5_C_INCLUDE_DIR )
	    get_filename_component( hdf5_path ${HDF5_C_INCLUDE_DIR} DIRECTORY )
	elseif( HDF5_INCLUDE_DIR )
	    list(GET HDF5_INCLUDE_DIR 0 HDF5_FIRST_INCLUDE_DIR )
	    get_filename_component( hdf5_path ${HDF5_FIRST_INCLUDE_DIR} DIRECTORY )
	else()
	    list(GET HDF5_INCLUDE_DIRS 0 HDF5_FIRST_INCLUDE_DIR )
	    get_filename_component( hdf5_path ${HDF5_FIRST_INCLUDE_DIR} DIRECTORY )
	endif()
    elseif( WIN32 )
	set( hdf5_path "" )
    else()
	set( hdf5_path "/usr" )
    endif()
endmacro(GET_HDF5_ROOT)

macro( SETHDF5DIR )
    if ( DEFINED HDF5_ROOT )
	if ( IS_DIRECTORY "${HDF5_ROOT}/cmake/hdf5" )
	    set( HDF5_DIR "${HDF5_ROOT}/cmake/hdf5" )
	elseif( IS_DIRECTORY "${HDF5_ROOT}/share/cmake/hdf5" )
	    set( HDF5_DIR "${HDF5_ROOT}/share/cmake/hdf5" )
	endif()
    endif()
endmacro(SETHDF5DIR)

macro( OD_GET_LINKLIBS )
    if( NOT HDF5_CXX_SHARED_LIBRARY AND HDF5_CXX_LIBRARY_hdf5_cpp )
	set( HDF5_CXX_SHARED_LIBRARY ${HDF5_CXX_LIBRARY_hdf5_cpp} )
    endif()
    if( NOT HDF5_C_SHARED_LIBRARY AND HDF5_CXX_LIBRARY_hdf5 )
	set( HDF5_C_SHARED_LIBRARY ${HDF5_CXX_LIBRARY_hdf5} )
    endif()
endmacro(OD_GET_LINKLIBS)

macro( OD_FIND_HDF5 )

    if ( NOT DEFINED HDF5_ROOT )
	OD_CLEANUP_HDF5()
    elseif ( DEFINED HDF5_DIFF_EXECUTABLE OR DEFINED HDF5_C_INCLUDE_DIR OR DEFINED HDF5_INCLUDE_DIRS )
	GET_HDF5_ROOT()
	if ( NOT IS_DIRECTORY "${HDF5_ROOT}" )
	    set( HDF5_ROOT "${hdf5_path}" )
	    set( HDF5_FOUND TRUE )
	elseif ( NOT "${HDF5_ROOT}" STREQUAL "${hdf5_path}" )
	    OD_CLEANUP_HDF5()
	endif()
    endif()

    if ( NOT HDF5_FOUND )
	SETHDF5DIR()
	set( HDF5_USE_STATIC_LIBRARIES False )
	find_package( HDF5 QUIET NAMES hdf5 COMPONENTS C CXX shared GLOBAL )
	if ( NOT HDF5_FOUND )
	    find_package( HDF5 QUIET COMPONENTS C CXX GLOBAL )
	    if ( "${HDF5_CXX_LIBRARIES}" STREQUAL "" )
		unset( HDF5_FOUND )
	    elseif ( UNIX AND NOT APPLE )
		if ( NOT HDF5_C_SHARED_LIBRARY AND HDF5_C_LIBRARIES )
		    set( HDF5_C_SHARED_LIBRARY "${HDF5_C_LIBRARIES}" )
		endif()
		if ( NOT HDF5_CXX_SHARED_LIBRARY AND HDF5_CXX_LIBRARIES )
		    list( REMOVE_ITEM HDF5_CXX_LIBRARIES "${HDF5_C_LIBRARIES}" )
		    set( HDF5_CXX_SHARED_LIBRARY "${HDF5_CXX_LIBRARIES}" )
		endif()
	    endif()
	endif()
	if ( HDF5_FOUND )
	    GET_HDF5_ROOT()
	    set( HDF5_ROOT "${hdf5_path}" )
	    if ( NOT HDF5_DIR )
		SETHDF5DIR()
		OD_CLEANUP_HDF5()
		UNSET( HDF5_FOUND )
		find_package( HDF5 QUIET NAMES hdf5 COMPONENTS C CXX shared GLOBAL )
		if ( NOT HDF5_FOUND )
		    find_package( HDF5 QUIET COMPONENTS C CXX GLOBAL )
		    if ( "${HDF5_CXX_LIBRARIES}" STREQUAL "" )
			unset( HDF5_FOUND )
		    endif()
		endif()
	    endif()
	    if ( TARGET hdf5::hdf5-shared )
		od_map_configurations( hdf5::hdf5-shared )
	    endif()
	    if ( TARGET hdf5::hdf5_cpp-shared )
		od_map_configurations( hdf5::hdf5_cpp-shared )
	    endif()
	    unset( HDF5_ROOT CACHE )
	else()
	    OD_CLEANUP_HDF5()
	endif()
	if ( NOT HDF5_DIR )
	    unset( HDF5_DIR CACHE )
	endif()
    endif()

endmacro(OD_FIND_HDF5)

macro( OD_SETUP_HDF5 )

    if ( HDF5_FOUND AND TARGET hdf5::hdf5-shared AND TARGET hdf5::hdf5_cpp-shared )
	OD_GET_LINKLIBS()
	list ( APPEND OD_MODULE_EXTERNAL_LIBS
		hdf5::hdf5-shared
		hdf5::hdf5_cpp-shared )

	if ( WIN32 )
	    GETHDF5COMPDEF()
	    list( APPEND OD_MODULE_COMPILE_DEFINITIONS "${HDF5_COMPILEDEF}" )
	    if ( HDF5_VERSION VERSION_GREATER_EQUAL 1.12 AND
		 CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" )
		list( APPEND OD_MODULE_COMPILE_OPTIONS "/wd4268" )
	    endif()
	endif()
    else()
	set( HDF5_ROOT "" CACHE PATH "HDF5 Location" )
    endif()

endmacro(OD_SETUP_HDF5)
