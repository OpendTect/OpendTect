#_______________________Pmake___________________________________________________
#
#       CopyRight:	dGB Beheer B.V.
#       July 2018	Bert/Arnaud
#_______________________________________________________________________________

list( APPEND HDF5LIBS hdf5 hdf5_cpp )
list( APPEND HDF5MOD RELEASE DEBUG )

# cmake <=3.7 sets different variable names.
macro( HDF5CLEANUP )
  foreach( LIB IN LISTS HDF5LIBS )
    foreach( HMOD IN LISTS HDF5MOD )
      if ( DEFINED HDF5_${LIB}_LIBRARY_${HMOD} )
	if( "${HMOD}" MATCHES "RELEASE" )
	  set( HDF5_CXX_LIBRARY_${LIB} ${HDF5_${LIB}_LIBRARY_${HMOD}} CACHE FILEPATH "Path to a library." )
	endif()
	unset( HDF5_${LIB}_LIBRARY_${HMOD} CACHE )
      endif()
    endforeach()
  endforeach()
  unset( HDF5_CXX_INCLUDE_DIR CACHE )
endmacro( HDF5CLEANUP )

HDF5CLEANUP()

if ( EXISTS ${HDF5_CXX_LIBRARY_hdf5} )
  get_filename_component( hdf5_path ${HDF5_CXX_LIBRARY_hdf5} DIRECTORY )
  if ( IS_DIRECTORY ${hdf5_path} )
    get_filename_component( hdf5_path ${hdf5_path} DIRECTORY )
    if ( IS_DIRECTORY ${hdf5_path} )
      set ( ENV{HDF5_ROOT} ${hdf5_path} )
    endif()
  endif()
endif()

if ( DEFINED HDF5_ROOT AND NOT ${HDF5_ROOT} MATCHES "$ENV{HDF5_ROOT}" )
  foreach( LIB IN LISTS HDF5LIBS )
    unset( HDF5_CXX_LIBRARY_${LIB} CACHE )
  endforeach()
  unset( HDF5_CXX_COMPILER_EXECUTABLE CACHE )
  unset( HDF5_DIFF_EXECUTABLE CACHE )
  set ( ENV{HDF5_ROOT} ${HDF5_ROOT} )
endif()

find_package( HDF5 1.10 QUIET COMPONENTS CXX )
if ( HDF5_FOUND )
  HDF5CLEANUP()
  if ( OD_INSTALL_DEPENDENT_LIBS )
    if ( UNIX )
      foreach( LIB IN LISTS HDF5LIBS )
	OD_INSTALL_LIBRARY( ${HDF5_CXX_LIBRARY_${LIB}} ${CMAKE_BUILD_TYPE} )
      endforeach()
    else()
      foreach( LIB IN LISTS HDF5LIBS )
	get_filename_component( HDF5_LIBNM ${HDF5_CXX_LIBRARY_${LIB}} NAME_WE )
	get_filename_component( HDF5_BASEDIR ${HDF5_CXX_LIBRARY_${LIB}} DIRECTORY )
        get_filename_component( HDF5_BASEDIR ${HDF5_BASEDIR} DIRECTORY )
        OD_INSTALL_LIBRARY( ${HDF5_BASEDIR}/bin/${HDF5_LIBNM}.dll ${CMAKE_BUILD_TYPE} )
      endforeach()
    endif()
  endif(OD_INSTALL_DEPENDENT_LIBS)
endif()

if ( HDF5_ROOT )
  unset( HDF5_ROOT CACHE )
endif()
