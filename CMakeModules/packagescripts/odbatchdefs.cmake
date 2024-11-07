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

set( SYSTEMLIBS libstdc++.so.6 libgcc_s.so.1 )
set( EXTERNAL_BACKEND_FILES libcrypto.so.3 libhdf5_cpp.so.310 libhdf5.so.310 libicudata.so.56 libicui18n.so.56 libicuuc.so.56 libproj.so.25 libQt5Core.so.5 libQt5Network.so.5 libQt5Sql.so.5 libsqlite3.so.0 libssl.so.3 libz.so.1 qt.conf )

set( PLUGINS CEEMDAttrib;CRS;ExpAttribs;GLCM;ODHDF5;)

set( EXECLIST od_isopach;od_remoteservice;od_remexec;od_copy_seis;od_process_2dto3d;od_process_time2depth;od_process_segyio;od_process_velocityconv;od_process_volume;od_process_prestack;od_process_attrib;od_process_2dgrid;od_process_attrib_em;od_stratamp;)

set( PACK "odbatch" )
