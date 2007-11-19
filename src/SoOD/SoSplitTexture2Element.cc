/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          November 2007
 RCS:           $Id: SoSplitTexture2Element.cc,v 1.1 2007-11-19 13:50:16 cvskris Exp $
________________________________________________________________________

-*/

#include "SoSplitTexture2Element.h"

SO_ELEMENT_SOURCE( SoSplitTexture2Element );

void SoSplitTexture2Element::initClass()
{
    SO_ELEMENT_INIT_CLASS( SoSplitTexture2Element, SoTextureImageElement );
}


SoSplitTexture2Element::~SoSplitTexture2Element()
{}

void SoSplitTexture2Element::set( SoState* const state, SoNode* node,
	                          const SbVec2s & size, const int numComponents,
				  const unsigned char * bytes, const Wrap wrapS,
				  const Wrap wrapT, const Model model,
				  const SbColor& blendColor)
{
    SoSplitTexture2Element * elem =
	(SoSplitTexture2Element *) state->getElement(classStackIndex);
    if ( elem )
    {
	elem->setElt(size, numComponents, bytes, wrapS, wrapT,
		     model, blendColor);
    }
}

const unsigned char*
SoSplitTexture2Element::getImage( const SoState* state,
                                  SbVec2s &size,
                                  int &numComponents)
{
    SoSplitTexture2Element * elem =
	(SoSplitTexture2Element*) state->getConstElement(classStackIndex);

    size.setValue( elem->size[0], elem->size[1] );
    numComponents = elem->numComponents;
    return elem->bytes;
}
