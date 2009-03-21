/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          November 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: SoTextureAnisotropicFilterElement.cc,v 1.1 2009-03-21 02:05:05 cvskris Exp $";

#include "SoTextureAnisotropicFilterElement.h"

#include <Inventor/elements/SoGLCacheContextElement.h>

#include <Inventor/system/gl.h>
#include <Inventor/C/glue/gl.h>

#include <string.h>

SO_ELEMENT_SOURCE( SoTextureAnisotropicFilterElement );

void SoTextureAnisotropicFilterElement::initClass()
{
    SO_ELEMENT_INIT_CLASS( SoTextureAnisotropicFilterElement,
	    		   SoReplacedElement );
}


SoTextureAnisotropicFilterElement::~SoTextureAnisotropicFilterElement()
{}


bool SoTextureAnisotropicFilterElement::isCapable()
{
    static char iscapable = 0; //0 - dont know, -1 = no, 1 = yes
    if  ( !iscapable )
    {
	if ( !strstr((char*)glGetString(GL_EXTENSIONS), 
			"GL_EXT_texture_filter_anisotropic"))
	    iscapable = -1;
	else
	    iscapable = 1;
    }

    return iscapable==1;
}


float SoTextureAnisotropicFilterElement::maxAnisotropy()
{
    static bool isset = false;
    static GLfloat largest_supported_anisotropy;
    if ( !isset )
    {
	isset = true;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT,
		    &largest_supported_anisotropy);
    }

    return largest_supported_anisotropy;
}


void SoTextureAnisotropicFilterElement::set( SoState* const state, SoNode* node,
					     float maxanisotropy )
{
    if ( !isCapable() )
	return;

    SoTextureAnisotropicFilterElement * elem =
	(SoTextureAnisotropicFilterElement *)
	    getElement( state, classStackIndex, node );

    if ( elem ) elem->setElt( maxanisotropy );
}


void SoTextureAnisotropicFilterElement::setElt( float maxanisotropy )
{
    const float limit = maxAnisotropy();
    if ( maxanisotropy<1 )
	curmaxanisotropy_ = 1;
    else if ( maxanisotropy>limit )
	curmaxanisotropy_ = maxanisotropy;
    else
	curmaxanisotropy_ = limit;

    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
		     curmaxanisotropy_ );
}
