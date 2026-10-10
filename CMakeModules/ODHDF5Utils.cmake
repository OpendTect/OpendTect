#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
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
	find_package( HDF5 QUIET COMPONENTS C CXX shared static CONFIG GLOBAL PATHS "${HDF5_ROOT}" HINTS "${CMAKE_PREFIX_PATH}" NO_DEFAULT_PATH )
	if ( NOT ((TARGET hdf5::hdf5-shared AND TARGET hdf5::hdf5_cpp-shared) OR
		  (TARGET hdf5::hdf5-static AND TARGET hdf5::hdf5_cpp-static)) )
	    if ( CMAKE_PREFIX_PATH )
		set( _OD_HDF5_ROOT "${HDF5_ROOT}" )
		set( HDF5_ROOT "${CMAKE_PREFIX_PATH}" )
		find_package( HDF5 QUIET MODULE COMPONENTS C CXX GLOBAL )
		unset( HDF5_DIR CACHE )
		set( HDF5_ROOT "${_OD_HDF5_ROOT}" )
		unset( _OD_HDF5_ROOT )
	    endif()
	    if ( NOT ((TARGET hdf5::hdf5-shared AND TARGET hdf5::hdf5_cpp-shared) OR
		      (TARGET hdf5::hdf5-static AND TARGET hdf5::hdf5_cpp-static)) )
		find_package( HDF5 QUIET COMPONENTS C CXX GLOBAL HINTS "${CMAKE_PREFIX_PATH}" )
		unset( HDF5_DIR CACHE )
	    endif()
	    if ( TARGET hdf5::hdf5 )
		od_setup_external_target( hdf5::hdf5 )
		if ( NOT TARGET hdf5::hdf5-shared AND NOT TARGET hdf5::hdf5-static )
		    add_library( hdf5::hdf5-shared ALIAS hdf5::hdf5 )
		endif()
	    endif()
	    if ( TARGET hdf5::hdf5_cpp )
		od_setup_external_target( hdf5::hdf5_cpp )
		if ( NOT TARGET hdf5::hdf5_cpp-shared AND NOT TARGET hdf5::hdf5_cpp-static )
		    add_library( hdf5::hdf5_cpp-shared ALIAS hdf5::hdf5_cpp )
		endif()
	    endif()
	elseif ( TARGET hdf5::hdf5-shared AND TARGET hdf5::hdf5_cpp-shared )
	    od_setup_external_target( hdf5::hdf5-shared )
	    od_setup_external_target( hdf5::hdf5_cpp-shared )
	elseif ( TARGET hdf5::hdf5-static AND TARGET hdf5::hdf5_cpp-static )
	    od_setup_external_target( hdf5::hdf5-static )
	    od_setup_external_target( hdf5::hdf5_cpp-static )
	endif()
	if ( (TARGET hdf5::hdf5-shared AND TARGET hdf5::hdf5_cpp-shared) OR
	     (TARGET hdf5::hdf5-static AND TARGET hdf5::hdf5_cpp-static) )
	    unset( HDF5_ROOT CACHE )
	endif()
    endif()

endmacro(OD_FIND_HDF5)

macro( OD_SETUP_HDF5 )

    if ( TARGET hdf5::hdf5-shared AND TARGET hdf5::hdf5_cpp-shared )
	list ( APPEND OD_MODULE_EXTERNAL_LIBS hdf5::hdf5-shared
					      hdf5::hdf5_cpp-shared )
    elseif ( TARGET hdf5::hdf5-static AND TARGET hdf5::hdf5_cpp-static )
	list ( APPEND OD_MODULE_EXTERNAL_LIBS hdf5::hdf5-static
					      hdf5::hdf5_cpp-static )
    endif()

    if ( (TARGET hdf5::hdf5-shared AND TARGET hdf5::hdf5_cpp-shared) OR
	 (TARGET hdf5::hdf5-static AND TARGET hdf5::hdf5_cpp-static) )
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
