#________________________________________________________________________
#
# Copyright:    dGB Beheer B.V.
# License:      https://dgbes.com/index.php/licensing
#________________________________________________________________________
#
# CMake script to define base package variables
#

#OpendTect libraries
set( LIBLIST Basic;Algo;General;Database;Strat;Network;Batch;Geometry;EarthModel;Well;MMProc;Seis;NLA;AttributeEngine;Velocity;VolumeProcessing;PreStackProcessing;Attributes;MPEEngine;EMAttrib;WellAttrib)

set( TXTFILES GNU_GENERAL_PUBLIC_LICENSE.txt INSTALL.txt LICENSE.txt )
if( WIN32 )
    set( SPECFILES ${TXTFILES} )
    set( ODSCRIPTS od_external.bat )
else()
    set( SPECFILES .exec_prog .init_dtect .init_dtect_user install
		   .lic_inst_common .lic_start_common mk_datadir setup.od )
    set( SPECFILES ${SPECFILES} ${TXTFILES} )
    set( ODSCRIPTS od_* mksethdir )
    list( REMOVE_ITEM ODSCRIPTS od_external.bat )
endif()

set( SYSTEMLIBS stdc++;gcc_s )
set( EXTERNAL_BACKEND_LIBS hdf5_cpp;hdf5;icudata;icui18n;icuuc;proj;Qt5Core;Qt5Network;Qt5Sql;sqlite3;z )
set( OPENSSLLIBS crypto;ssl )

set( PLUGINS CEEMDAttrib;CRS;ExpAttribs;GLCM;ODHDF5;)

set( EXECLIST od_isopach;od_remoteservice;od_remexec;od_copy_seis;od_process_2dto3d;od_process_time2depth;od_process_segyio;od_process_velocityconv;od_process_volume;od_process_prestack;od_process_attrib;od_process_2dgrid;od_process_attrib_em;od_stratamp;)

set( PACK "odbatch" )
