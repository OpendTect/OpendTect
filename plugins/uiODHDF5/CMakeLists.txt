#_______________________Pmake___________________________________________________
#
#	Makefile : 	HDF5 UI
# 	May 2018	Bert
#_______________________________________________________________________________

OD_SETUP_HDF5()

if ( HDF5_FOUND )

    set( OD_MODULE_DEPS uiTools ODHDF5 )
    set( OD_IS_PLUGIN yes )
    set( OD_MODULE_SOURCES
      uihdf5settings.cc
      uiodhdf5pi.cc
    )

    set( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} )

    OD_INIT_MODULE()

endif()
