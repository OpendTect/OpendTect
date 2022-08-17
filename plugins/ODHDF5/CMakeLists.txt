#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#

if ( HDF5_FOUND AND NOT "${HDF5_ROOT}" STREQUAL "" )

  set( OD_IS_PLUGIN TRUE )
  set( OD_MODULE_DEPS General )
  set( OD_USEHDF5 Yes )

  set( OD_TEST_PROGS hdf5.cc )

  set( OD_MODULE_SOURCES
      hdf5accessimpl.cc
      hdf5readerimpl.cc
      hdf5writerimpl.cc
      odhdf5pi.cc
  )

  set ( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} ${OD_ATTRIB_EXECS} ${OD_VOLUME_EXECS}
			 ${OD_SEIS_EXECS} ${OD_PRESTACK_EXECS}
			 ${OD_ZAXISTRANSFORM_EXECS} test_hdf5 )

  OD_INIT_MODULE()

else()
  get_filename_component( OD_MODULE_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME )
  list( REMOVE_ITEM OD_PLUGINS ${OD_MODULE_NAME} )
  set( OD_PLUGINS ${OD_PLUGINS} PARENT_SCOPE )
endif()
