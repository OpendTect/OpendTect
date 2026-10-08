#________________________________________________________________________
#
# Copyright:	(C) 1995-2026 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

macro( GETHDF5COMPDEF )

    set( HDF5_COMPILEDEF "" )
    if ( TARGET hdf5::hdf5-shared )
	get_target_property( HDF5_COMPILEDEF hdf5::hdf5-shared INTERFACE_COMPILE_DEFINITIONS )
	if ( NOT HDF5_COMPILEDEF )
	    set( HDF5_COMPILEDEF "H5_BUILT_AS_DYNAMIC_LIB" )
	endif()
    elseif ( TARGET hdf5::hdf5-static )
	get_target_property( HDF5_COMPILEDEF hdf5::hdf5-static INTERFACE_COMPILE_DEFINITIONS )
	if ( NOT HDF5_COMPILEDEF )
	    set( HDF5_COMPILEDEF "" )
	endif()
    endif()

endmacro(GETHDF5COMPDEF)

macro( OD_FIND_HDF5 )

    if ( NOT HDF5_FOUND )
	find_package( HDF5 QUIET COMPONENTS C shared static CONFIG GLOBAL PATHS "${HDF5_ROOT}" HINTS "${CMAKE_PREFIX_PATH}" NO_DEFAULT_PATH )
	if ( NOT TARGET hdf5::hdf5-shared AND NOT TARGET hdf5::hdf5-static )
	    if ( CMAKE_PREFIX_PATH )
		set( _OD_HDF5_ROOT "${HDF5_ROOT}" )
		set( HDF5_ROOT "${CMAKE_PREFIX_PATH}" )
		find_package( HDF5 QUIET MODULE COMPONENTS C CXX GLOBAL )
		unset( HDF5_DIR CACHE )
		set( HDF5_ROOT "${_OD_HDF5_ROOT}" )
		unset( _OD_HDF5_ROOT )
	    endif()
	    if ( NOT TARGET hdf5::hdf5-shared AND NOT TARGET hdf5::hdf5-static )
		find_package( HDF5 QUIET COMPONENTS C GLOBAL HINTS "${CMAKE_PREFIX_PATH}" )
		unset( HDF5_DIR CACHE )
	    endif()
	    if ( TARGET hdf5::hdf5 )
		od_setup_external_target( hdf5::hdf5 )
		if ( NOT TARGET hdf5::hdf5-shared AND NOT TARGET hdf5::hdf5-static )
		    add_library( hdf5::hdf5-shared ALIAS hdf5::hdf5 )
		endif()
	    endif()
	elseif ( TARGET hdf5::hdf5-shared )
	    od_setup_external_target( hdf5::hdf5-shared )
	elseif ( TARGET hdf5::hdf5-static )
	    od_setup_external_target( hdf5::hdf5-static )
	endif()
	if ( TARGET hdf5::hdf5-shared OR TARGET hdf5::hdf5-static )
	    unset( HDF5_ROOT CACHE )
	endif()
    endif()

endmacro(OD_FIND_HDF5)

macro( OD_SETUP_HDF5 )

    if ( TARGET hdf5::hdf5-shared )
	list ( APPEND OD_MODULE_EXTERNAL_LIBS hdf5::hdf5-shared )
    elseif ( TARGET hdf5::hdf5-static )
	list ( APPEND OD_MODULE_EXTERNAL_LIBS hdf5::hdf5-static )
    endif()

    if ( TARGET hdf5::hdf5-shared OR TARGET hdf5::hdf5-static )
	if ( WIN32 )
	    GETHDF5COMPDEF()
	    if ( HDF5_COMPILEDEF )
		list( APPEND OD_MODULE_COMPILE_DEFINITIONS "${HDF5_COMPILEDEF}" )
	    endif()
	    if ( HDF5_VERSION VERSION_GREATER_EQUAL 1.12 AND
		 CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" )
		list( APPEND OD_MODULE_COMPILE_OPTIONS "/wd4268" )
	    endif()
	endif()
    else()
	set( HDF5_ROOT "" CACHE PATH "HDF5 Location" )
    endif()

endmacro(OD_SETUP_HDF5)
