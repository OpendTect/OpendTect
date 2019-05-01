#_______________________Pmake___________________________________________________
#
#       CopyRight:	dGB Beheer B.V.
#       July 2018	Bert/Arnaud
#_______________________________________________________________________________

macro( HDF5CLEANUP )
  unset( HDF5_CXX_INCLUDE_DIR CACHE )
  unset( HDF5_CXX_COMPILER_EXECUTABLE CACHE )
  unset( HDF5_CXX_LIBRARY_hdf5 CACHE )
  unset( HDF5_CXX_LIBRARY_hdf5_cpp CACHE )
  unset( HDF5_DIFF_EXECUTABLE CACHE )
  unset( HDF5_IS_PARALLEL CACHE )
  unset( HDF5_hdf5_LIBRARY_DEBUG CACHE )
  unset( HDF5_hdf5_LIBRARY_RELEASE CACHE )
  unset( HDF5_hdf5_cpp_LIBRARY_DEBUG CACHE )
  unset( HDF5_hdf5_cpp_LIBRARY_RELEASE CACHE )
endmacro( HDF5CLEANUP )

macro( GETHDF5COMPDEF )
  find_package( HDF5 1.10 QUIET COMPONENTS CXX )
  get_target_property( HDF5_COMPILEDEF ${HDF5_CXX_LIBRARY_hdf5_cpp} INTERFACE_COMPILE_DEFINITIONS )
endmacro( GETHDF5COMPDEF )

macro( OD_FIND_HDF5 )

  find_package( HDF5 1.10 QUIET COMPONENTS C CXX )
  if ( NOT HDF5_FOUND )
    HDF5CLEANUP()
    return()
  endif()

endmacro( OD_FIND_HDF5 )


macro( OD_SETUP_HDF5 )

  if ( NOT HDF5_FOUND )
    return()
  endif()

  get_filename_component( HDF5_RUNTIMEDIR ${HDF5_CXX_LIBRARY_hdf5_cpp} DIRECTORY )
  list( APPEND OD_${OD_MODULE_NAME}_RUNTIMEPATH ${HDF5_RUNTIMEDIR} )
  foreach( LIB IN LISTS HDF5_LIBRARIES )
    get_target_property( HDF5_LIB ${LIB} IMPORTED_LOCATION_${CMAKE_BUILD_TYPE} )
    if ( HDF5_LIB AND EXISTS ${HDF5_LIB} )
	list( APPEND OD_${OD_MODULE_NAME}_PLUGIN_EXTERNAL_DLL ${HDF5_LIB} )
	if ( OD_INSTALL_DEPENDENT_LIBS )
	  OD_INSTALL_LIBRARY( ${HDF5_LIB} ${CMAKE_BUILD_TYPE} )
	endif(OD_INSTALL_DEPENDENT_LIBS)
    endif()
  endforeach()

endmacro( OD_SETUP_HDF5 )
