#_______________________Pmake___________________________________________________
#
#	Makefile : 	HDF5 UI
# 	May 2018	Bert
#_______________________________________________________________________________

set( OD_MODULE_DEPS uiTools HDF5 )
set( OD_IS_PLUGIN yes )
set( OD_MODULE_SOURCES
	uihdf5settings.cc
	uihdf5pi.cc
)

set( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )
OD_INIT_MODULE()
