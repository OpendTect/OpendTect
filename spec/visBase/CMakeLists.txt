#_______________________Pmake___________________________________________________
#
#	Makefile : 	visBase
# 	May 2012 :	Nageswara
#	RCS :		$Id$
#_______________________________________________________________________________

if ( UNIX )
    set( EXTRA_LIBS ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} )
    if ( APPLE )
    else()
	list( APPEND EXTRA_LIBS X11 )
    endif( APPLE )
#    set( OD_MODULE_EXECS od_glxinfo )
    set( OD_MODULE_PROGS od_glxinfo.c )
else()
    set( OD_MODULE_EXECS w32glxinfo.c )
endif( UNIX )

OD_INIT_MODULE()
