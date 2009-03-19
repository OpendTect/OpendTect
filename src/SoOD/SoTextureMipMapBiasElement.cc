/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          November 2007
________________________________________________________________________

-*/
static const char* rcsID = "$Id: SoTextureMipMapBiasElement.cc,v 1.1 2009-03-19 14:46:30 cvskris Exp $";

#include "SoTextureMipMapBiasElement.h"

#include <Inventor/elements/SoGLCacheContextElement.h>

#include <Inventor/system/gl.h>
#include <Inventor/C/glue/gl.h>


SO_ELEMENT_SOURCE( SoTextureMipMapBiasElement );

void SoTextureMipMapBiasElement::initClass()
{
    SO_ELEMENT_INIT_CLASS( SoTextureMipMapBiasElement, SoReplacedElement );
}


SoTextureMipMapBiasElement::~SoTextureMipMapBiasElement()
{}


void SoTextureMipMapBiasElement::set( SoState* const state, SoNode* node,
       				      float bias )
{
    static int iscapable = 0; //0 - dont know, -1 = no, 1 = yes
    if  ( !iscapable )
    {
	const cc_glglue* glue =
	    cc_glglue_instance(SoGLCacheContextElement::get(state));
	if ( glue )
	{
	    iscapable = cc_glglue_glversion_matches_at_least(glue, 1, 4, 0)
		? 1
		: -1;
	}
    }

    if ( iscapable==1 )
    {
	SoTextureMipMapBiasElement * elem = (SoTextureMipMapBiasElement *)
	    getElement( state, classStackIndex, node );

	if ( elem ) elem->setElt( bias );
    }
}


void SoTextureMipMapBiasElement::setElt( float bias )
{
    bias_ = bias;
    glTexEnvf( GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, bias_ );
}
