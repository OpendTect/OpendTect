#_______________________Pmake___________________________________________________
#
#	Makefile : 	Seis Pre-ModInit
# 	Feb 2018	Bert
#_______________________________________________________________________________

list( APPEND OD_MODULE_INCLUDESYSPATH ${HDF5_INCLUDE_DIRS} )
list( APPEND OD_MODULE_EXTERNAL_LIBS ${HDF5_LIBRARIES} )

if ( WIN32 )
  GETHDF5COMPDEF()
  add_definitions( -D${HDF5_COMPILEDEF} )
endif()
