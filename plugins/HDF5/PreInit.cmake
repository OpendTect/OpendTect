#_______________________Pmake___________________________________________________
#
#       Makefile:	HDF5 global PreInit
#       Feb 2018	Bert
#_______________________________________________________________________________


if ( NOT EXISTS ${HDF5_hdf5_LIBRARY_RELEASE} )
  find_package( HDF5 1.10 QUIET COMPONENTS CXX )
  if ( HDF5_FOUND )
    set( OD_PLUGINS ${OD_PLUGINS} HDF5 )
    if ( HDF5_ROOT )
      unset( HDF5_ROOT CACHE )
    endif()
  endif()
else()
  set( OD_PLUGINS ${OD_PLUGINS} HDF5 )
endif()


