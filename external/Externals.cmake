#________________________________________________________________________
#
# Copyright:    (C) 1995-2022 dGB Beheer B.V.
# License:      https://dgbes.com/licensing
#________________________________________________________________________
#

include ( "CMakeModules/ODExternals.cmake" )

DEFINE_GIT_EXTERNAL( odpy https://github.com/OpendTect/odpy.git od7.0 )
if ( BUILD_PROJ )
    DEFINE_GIT_EXTERNAL( proj https://github.com/OSGeo/PROJ.git 9.0.1 )
endif()

if ( NOT OD_NO_OSG )
    DEFINE_GIT_EXTERNAL( osgGeo https://github.com/OpendTect/osgGeo.git osgGeo-1.5 )
endif()
