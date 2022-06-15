include ( "CMakeModules/ODExternals.cmake" )

DEFINE_GIT_EXTERNAL( odpy https://github.com/OpendTect/odpy.git main )
if ( BUILD_PROJ )
    DEFINE_GIT_EXTERNAL( proj https://github.com/OSGeo/PROJ.git 9.0.0 )
endif()

if ( NOT OD_NO_OSG )
    DEFINE_GIT_EXTERNAL( osgGeo https://github.com/OpendTect/osgGeo.git main )
endif()
