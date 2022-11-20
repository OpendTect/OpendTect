#________________________________________________________________________
#
# Copyright:	(C) 1995-2022 dGB Beheer B.V.
# License:	https://dgbes.com/licensing
#________________________________________________________________________
#

if ( UNIX )
  set( OpenGL_GL_PREFERENCE "LEGACY" )
endif()
find_package( OpenGL REQUIRED )

if ( NOT APPLE )
    set( EXTRA_LIBS ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} )
    if ( UNIX )
	find_package( X11 )
	list( APPEND EXTRA_LIBS ${X11_LIBRARIES} )
    endif()
    set( OD_FOLDER "Base" )
    set( OD_MODULE_PROGS od_glxinfo.c )
endif()

OD_INIT_MODULE()
