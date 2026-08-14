#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

macro( GETHDF5COMPDEF )
    if ( TARGET ${HDF5_C_SHARED_LIBRARY} )
	get_target_property( HDF5_COMPILEDEF ${HDF5_C_SHARED_LIBRARY} INTERFACE_COMPILE_DEFINITIONS )
    endif()
    if ( NOT DEFINED HDF5_COMPILEDEF OR NOT HDF5_COMPILEDEF )
	set( HDF5_COMPILEDEF "H5_BUILT_AS_DYNAMIC_LIB" )
    endif()
endmacro(GETHDF5COMPDEF)

macro( OD_FIND_HDF5 )

    if ( NOT HDF5_FOUND )
	find_package( HDF5 CONFIG QUIET COMPONENTS C CXX CONFIG GLOBAL PATHS "${HDF5_ROOT}" HINTS "${CMAKE_PREFIX_PATH}" NO_DEFAULT_PATH )
	if ( NOT TARGET hdf5::hdf5-shared )
	    find_package( HDF5 QUIET COMPONENTS C GLOBAL )
	    unset( HDF5_DIR CACHE )
	    if ( TARGET hdf5::hdf5 )
		od_setup_external_target( hdf5::hdf5 )
		if ( NOT TARGET hdf5::hdf5-shared )
		    add_library( hdf5::hdf5-shared ALIAS hdf5::hdf5 )
		endif()
	    endif()
	else()
	    od_setup_external_target( hdf5::hdf5-shared )
	endif()
	if ( TARGET hdf5::hdf5-shared )
	    unset( HDF5_ROOT CACHE )
	endif()
    endif()

endmacro(OD_FIND_HDF5)

macro( OD_SETUP_HDF5 )

    if ( TARGET hdf5::hdf5-shared )
	list ( APPEND OD_MODULE_EXTERNAL_LIBS hdf5::hdf5-shared )
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
