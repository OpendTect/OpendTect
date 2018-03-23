include ( "CMakeModules/ODExternals.cmake" )

DEFINE_GIT_EXTERNAL( osgGeo https://github.com/OpendTect/osgGeo.git osgGeo-1.3 )
DEFINE_GIT_EXTERNAL( proj4 https://github.com/OpendTect/proj4.git master )
DEFINE_SVN_EXTERNAL( external/doc_csh https://github.com/opendtect/opendtectdoc.git/branches/od6.2/od_userdoc/Project/Advanced/CSH HEAD )
