/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: SoOD.cc,v 1.18 2007-12-14 05:15:23 cvssatyaki Exp $";


#include "SoOD.h"

#include "Inventor/nodes/SoFragmentShader.h"

#ifndef win
extern "C" { extern void* glXGetCurrentContext(); }
#endif

int SoOD::supportsFragShading()
{
    static int answer = 0;
    if ( !answer )
    {
#ifdef win
	if ( wglGetCurrentContext() )
#else
	if ( glXGetCurrentContext() )
#endif
	    answer = SoFragmentShader::isSupported(
		    		SoShaderObject::GLSL_PROGRAM) ? 1 : -1;
    }

    return answer;
}
