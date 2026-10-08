#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS General )
set( OD_FOLDER "OD Free Plugins" )
set( OD_IS_PLUGIN TRUE )
set( OD_USEHDF5 TRUE )

set( OD_TEST_PROGS hdf5.cc )

set( OD_MODULE_SOURCES
	hdf5accessimpl.cc
	hdf5readerimpl.cc
	hdf5writerimpl.cc
	odhdf5pi.cc
)

set ( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC}
	${OD_ATTRIB_EXECS} ${OD_VOLUME_EXECS}
	${OD_SEIS_EXECS} ${OD_PRESTACK_EXECS}
	${OD_ZAXISTRANSFORM_EXECS} ${OD_DB_EXECS} test_hdf5 )

OD_SETUP_HDF5()
if ( HDF5_FOUND AND TARGET hdf5::hdf5-shared AND TARGET hdf5::hdf5_cpp-shared )
    OD_INIT_MODULE()
else()

    get_filename_component( OD_MODULE_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME )
    list( REMOVE_ITEM OD_PLUGINS ${OD_MODULE_NAME} )
    set( OD_PLUGINS ${OD_PLUGINS} PARENT_SCOPE )

endif()
