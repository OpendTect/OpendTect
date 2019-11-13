include ( "CMakeModules/ODExternals.cmake" )

DEFINE_GIT_EXTERNAL( osgGeo https://github.com/OpendTect/osgGeo.git osgGeo-1.4 )
DEFINE_GIT_EXTERNAL( proj4 https://github.com/OpendTect/proj4.git 5.1.0 )
DEFINE_SVN_EXTERNAL( doc_csh https://github.com/OpendTect/opendtectdoc.git/branches/od6.4/od_userdoc/Project/Advanced/CSH ${CMAKE_SOURCE_DIR}/external 436 )
