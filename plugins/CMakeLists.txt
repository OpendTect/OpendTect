#_______________________Pmake__________________________________________________
#
#       CopyRight:      dGB Beheer B.V.
#       Jan 2012        K. Tingdahl
#       RCS :           $Id: RootMakefile,v 1.4 2010-09-28 07:21:07 cvsbert
#_______________________________________________________________________________

SET(PLUGINS
	My_Plugin_Name
	Tut )

INCLUDE ( ODdGBPlugins.cmake OPTIONAL )

FOREACH( PLUGIN ${PLUGINS} )
    add_subdirectory( ${PLUGIN} )
ENDFOREACH()
