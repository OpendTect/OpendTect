#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS EarthModel Seis Well )
set( OD_FOLDER "OD Free Plugins" )
set( OD_IS_PLUGIN yes )

set( OD_MODULE_SOURCES
    odbind_pi.cc
    odbind.cc
    odfault.cc
    odgeom_2d.cc
    odhorizon.cc
    odseismic_object.cc
    odseismic_3d.cc
    odseismic_2d.cc
    odsurvey.cc
    odsurvey_object.cc
    odwell.cc
    cubedataidx.cc
)

set( OD_PLUGIN_ALO_EXEC ${OD_MAIN_EXEC} ${DGB_ML_EXECS} ${DGB_ML_UIEXECS} )

OD_INIT_MODULE()

if ( EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/python" AND
     IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/python" )
    add_definitions( -D__odbind_dir__="${CMAKE_CURRENT_SOURCE_DIR}/python" )
    install ( DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/python/odbind"
	      DESTINATION "${MISC_INSTALL_PREFIX}/bin/python"
	      PATTERN ".*.swp" EXCLUDE
	      PATTERN "__pycache__" EXCLUDE
	      PATTERN ".gitignore" EXCLUDE
	      PATTERN "tests" EXCLUDE )
endif()
