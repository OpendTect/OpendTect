#_______________________Pmake___________________________________________________
#
#       Makefile:	HDF5 global PreInit
#       Feb 2018	Bert
#_______________________________________________________________________________

OD_SETUP_HDF5()

if ( HDF5_FOUND )
    set( OD_PLUGINS ${OD_PLUGINS} ODHDF5 )
endif()
