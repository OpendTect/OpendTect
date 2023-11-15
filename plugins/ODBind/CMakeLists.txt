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

OD_INIT_MODULE()

set ( ODBIND_FILES
    python/__init__.py
    python/survey.py
    python/geom2d.py
    python/horizon2d.py
    python/horizon3d.py
    python/seismic3d.py
    python/seismic2d.py
    python/well.py
)
if ( APPLE )
    install ( FILES ${ODBIND_FILES}
		DESTINATION Contents/Resources/bin/python/odbind )
else()
    install ( FILES ${ODBIND_FILES}
		DESTINATION bin/python/odbind )
endif()
