/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUnusedVar = "$Id$";


#include "SoOD.h"

#include "Inventor/nodes/SoFragmentShader.h"
#include "Inventor/nodes/SoVertexShader.h"
#include "Inventor/nodes/SoTextureUnit.h"

#ifdef __lux__
extern "C" { extern void* glXGetCurrentContext(); }
#elif __mac__
extern "C" { extern void* coin_gl_current_context(); }
#endif

bool SoOD::getAllParams()
{
    static bool answer = false;
    if ( !answer )
    {
#ifdef __win__
	if ( wglGetCurrentContext() )
#elif __lux__
	if ( glXGetCurrentContext() )
#else
	if ( coin_gl_current_context() )
#endif
	{
	    supportsFragShading();
	    supportsVertexShading();
	    maxNrTextureUnits();
	    maxTexture2DSize();
	    answer =  true;
	}
    }

    return answer;
}

int SoOD::supportsFragShading()
{
    static int answer = 0;
    if ( !answer )
    {
#ifdef __win__
	if ( wglGetCurrentContext() )
#elif __lux__
	if ( glXGetCurrentContext() )
#else
	if ( coin_gl_current_context() )
#endif
	{
	    answer =
		SoFragmentShader::isSupported( SoShaderObject::GLSL_PROGRAM)
		    ? 1 : -1;
	}
    }

    return answer;
}


int SoOD::supportsVertexShading()
{
    static int answer = 0;
    if ( !answer )
    {
#ifdef __win__
	if ( wglGetCurrentContext() )
#elif __lux__
	if ( glXGetCurrentContext() )
#else
	if ( coin_gl_current_context() )
#endif
	    answer = SoVertexShader::isSupported( SoShaderObject::GLSL_PROGRAM )
		    ? 1 : -1;
    }

    return answer;
}


int SoOD::maxNrTextureUnits()
{
    static int answer = 0;
    if ( !answer )
    {
#ifdef __win__
      if ( wglGetCurrentContext() )
#elif __lux__
      if ( glXGetCurrentContext() )
#else
      if ( coin_gl_current_context() )
#endif
          answer = SoTextureUnit::getMaxTextureUnit ();
    }

    return answer ? answer : 1;
}


int SoOD::maxTexture2DSize()
{
    static int answer = -1;
    if ( answer==-1 )
    {
#ifdef __win__
	if ( wglGetCurrentContext() )
#elif __lux__
	if ( glXGetCurrentContext() )
#else
	if ( coin_gl_current_context() )
#endif
	{
	    GLint maxr;
	    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxr);
	    if ( glGetError()==GL_NO_ERROR )
		answer = maxr;
	}
    }

    return answer==-1 ? 1024 : answer;
}


void SoOD::getLineWidthBounds( int& min, int& max )
{
    GLfloat bounds[2];
    glGetFloatv( GL_LINE_WIDTH_RANGE, bounds );
    min = (int)bounds[0];
    max = (int)bounds[1];
}

