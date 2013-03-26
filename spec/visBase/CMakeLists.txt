#_______________________Pmake___________________________________________________
#
#	Makefile : 	visBase
# 	May 2012 :	Nageswara
#	RCS :		$Id$
#_______________________________________________________________________________

IF ( UNIX )
    SET( EXTRA_LIBS ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} )
#    SET( OD_MODULE_EXECS od_glxinfo )
    SET( OD_MODULE_PROGS od_glxinfo.cc )
ELSE()
    SET( OD_MODULE_EXECS w32glxinfo.c )
ENDIF( UNIX )

OD_INIT_MODULE()
