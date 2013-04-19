#_______________________Pmake___________________________________________________
#
#	Makefile : 	visBase
# 	May 2012 :	Nageswara
#	RCS :		$Id$
#_______________________________________________________________________________

if ( UNIX )
    if ( APPLE )
    else()
	find_package( X11 )
	set( EXTRA_LIBS ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${X11_LIBRARIES} )
	set( OD_MODULE_PROGS od_glxinfo.c )
    endif( APPLE )
else()
    set( OD_MODULE_EXECS w32glxinfo.c )
endif( UNIX )

OD_INIT_MODULE()
