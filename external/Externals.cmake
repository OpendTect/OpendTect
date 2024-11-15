#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

include ( "CMakeModules/ODExternals.cmake" )

DEFINE_GIT_EXTERNAL( odpy https://github.com/OpendTect/odpy.git od8.0 )
DEFINE_GIT_EXTERNAL( safety https://github.com/pyupio/safety.git 2.3.5 TAG )
if ( NOT OD_NO_OSG )
    DEFINE_GIT_EXTERNAL( osgGeo https://github.com/OpendTect/osgGeo.git osgGeo-1.6 )
endif()
