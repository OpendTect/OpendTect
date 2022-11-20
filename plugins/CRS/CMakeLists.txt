#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

if ( NOT OD_NO_PROJ )
    set( OD_LINKPROJ Yes )
    set( OD_USESQLITE Yes )
endif()

set( OD_MODULE_DEPS General )
set( OD_FOLDER "OD Free Plugins" )
set( OD_IS_PLUGIN yes )
set( OD_MODULE_SOURCES 
	crspi.cc
	crsproj.cc
	crssystem.cc
)

if ( NOT OD_NO_PROJ )
    set( OD_TEST_PROGS crs.cc )
    set( OD_PLUGIN_ALO_EXEC
	  od_DBMan
	  od_WellMan
	  ${OD_ATTRIB_EXECS}
	  ${OD_VOLUME_EXECS}
	  ${OD_SEIS_EXECS}
	  ${OD_PRESTACK_EXECS}
	  ${OD_ZAXISTRANSFORM_EXECS}
	  od_gmtexec
	  od_process_flattening
	  ${DGB_PRO_EXECS}
	  ${DGB_ML_EXECS}
	  od_process_attrib_hc
	  od_synthrock_extractexamples
	  odpybind
    )
endif()

OD_INIT_MODULE()
