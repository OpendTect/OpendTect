include ( "CMakeModules/ODExternals.cmake" )

DEFINE_GIT_EXTERNAL( osgGeo https://github.com/OpendTect/osgGeo.git osgGeo-1.3 )
DEFINE_GIT_EXTERNAL( proj4 https://github.com/OpendTect/proj4.git 4.9.3 )
DEFINE_SVN_EXTERNAL( doc_csh https://github.com/OpendTect/opendtectdoc.git/branches/od6.2/od_userdoc/Project/Advanced/CSH ${CMAKE_SOURCE_DIR}/external HEAD )
