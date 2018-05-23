#_______________________Pmake___________________________________________________
#
#       Makefile:	HDF5 global PreInit
#       Feb 2018	Bert
#_______________________________________________________________________________

if ( EXISTS ${HDF5_hdf5_LIBRARY_RELEASE} )
  get_filename_component( hdf5_path ${HDF5_hdf5_LIBRARY_RELEASE} DIRECTORY )
  if ( IS_DIRECTORY ${hdf5_path} )
    get_filename_component( hdf5_path ${hdf5_path} DIRECTORY )
    if ( IS_DIRECTORY ${hdf5_path} )
      set ( HDF5_ROOT ${hdf5_path} )
    endif()
  endif()
endif()

find_package( HDF5 1.10 QUIET COMPONENTS CXX )
if ( HDF5_ROOT )
  unset( HDF5_ROOT CACHE )
endif()

if ( HDF5_FOUND )
  set( OD_PLUGINS ${OD_PLUGINS} HDF5 )
endif()
