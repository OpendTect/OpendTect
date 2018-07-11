#_______________________Pmake___________________________________________________
#
#       CopyRight:	dGB Beheer B.V.
#       July 2018	Bert/Arnaud
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
