/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: SoOD.cc,v 1.19 2009-03-23 20:11:21 cvskris Exp $";


#include "SoOD.h"

#include "Inventor/nodes/SoFragmentShader.h"

#ifdef lux
extern "C" { extern void* glXGetCurrentContext(); }
#elif mac
extern "C" { extern void* coin_gl_current_context(); }
#endif

int SoOD::supportsFragShading()
{
    static int answer = 0;
    if ( !answer )
    {
#ifdef win
	if ( wglGetCurrentContext() )
#elif lux
	if ( glXGetCurrentContext() )
#else
	if ( coin_gl_current_context() )
#endif
	    answer = SoFragmentShader::isSupported(
		    		SoShaderObject::GLSL_PROGRAM) ? 1 : -1;
    }

    return answer;
}
