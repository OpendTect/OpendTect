#_______________________Pmake___________________________________________________
#
#	Makefile : 	ODHDF5 Pre-ModInit
# 	Feb 2018	Bert
#_______________________________________________________________________________

list( APPEND OD_MODULE_INCLUDESYSPATH ${HDF5_INCLUDE_DIR} )
list( APPEND OD_MODULE_EXTERNAL_LIBS ${HDF5_LIBRARIES} )

if ( WIN32 )
  GETHDF5COMPDEF()
  add_definitions( -D${HDF5_COMPILEDEF} )
  if ( "${HDF5_VERSION}" STREQUAL "1.12.0" )
    set ( CMAKE_CXX_FLAGS "/wd4268 ${CMAKE_CXX_FLAGS}" )
  endif()
endif()

