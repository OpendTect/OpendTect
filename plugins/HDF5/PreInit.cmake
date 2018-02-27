#_______________________Pmake___________________________________________________
#
#       Makefile:	HDF5 global PreInit
#       Feb 2018	Bert
#_______________________________________________________________________________

find_package( HDF5 REQUIRED )

set( OD_PLUGINS ${OD_PLUGINS} HDF5 )
