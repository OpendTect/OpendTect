#_______________________Pmake___________________________________________________
#
#       Makefile:	HDF5 global PreInit
#       Feb 2018	Bert
#_______________________________________________________________________________

include( ODHDF5Utils )

if ( HDF5_FOUND )
  set( OD_PLUGINS ${OD_PLUGINS} HDF5 )
endif()
