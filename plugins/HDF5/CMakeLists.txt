#_______________________Pmake___________________________________________________
#
#	Makefile : 	OpenCL adaptions
# 	Mar 2016	K. Tingdahl
#_______________________________________________________________________________

if ( HDF5_FOUND )

include( PreModInit.cmake )

set( OD_IS_PLUGIN TRUE )
set( OD_MODULE_DEPS Basic )

set( OD_MODULE_SOURCES
    hdf5pi.cc
)

OD_INIT_MODULE()

endif( HDF5_FOUND )
