#_______________________Pmake___________________________________________________
#
#	Makefile : 	visBase
# 	May 2012 :	Nageswara
#	RCS :		$Id: CMakeLists.txt,v 1.1 2012/05/07 03:37:40 cvsnageswara Exp $
#_______________________________________________________________________________

IF ( UNIX )
    SET( EXTRA_LIBS ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} )
    SET( OD_MODULE_PROGS od_glxinfo )
ELSE()
    SET( OD_MODULE_EXECS w32glxinfo.c )
ENDIF( UNIX )

OD_INIT_MODULE()
