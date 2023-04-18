#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

set( OD_MODULE_DEPS EarthModel Well )

set( OD_FOLDER "OD Free Plugins" )
set( OD_IS_PLUGIN yes )
set( OD_MODULE_SOURCES
    odbind_pi.cc
    odbind.cc
    odhorizon.cc
    odsurvey.cc
    odsurvey_object.cc
    odwell.cc
)

OD_INIT_MODULE()

if ( APPLE )
    install ( FILES python/__init__.py python/odbind.py
		DESTINATION Contents/Resources/bin/python/ )
else()
    install ( FILES python/__init__.py python/odbind.py
		DESTINATION bin/python/ )
endif()
