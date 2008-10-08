/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          November 2007
 RCS:           $Id: SoTextureComposerElement.cc,v 1.1 2008-10-08 15:39:46 cvskris Exp $
________________________________________________________________________

-*/

#include "SoTextureComposerElement.h"

SO_ELEMENT_SOURCE( SoTextureComposerElement );

void SoTextureComposerElement::initClass()
{
    SO_ELEMENT_INIT_CLASS( SoTextureComposerElement, SoReplacedElement );
}


SoTextureComposerElement::~SoTextureComposerElement()
{}


void SoTextureComposerElement::set( SoState* const state, SoNode* node,
       				    const SbList<int>& units )
{
    SoTextureComposerElement * elem = (SoTextureComposerElement *)
	getElement( state, classStackIndex, node );

    if ( elem ) elem->setElt( units );
}


void SoTextureComposerElement::setElt( const SbList<int>& units )
{ units_ = units; }


const SbList<int>& SoTextureComposerElement::getUnits( SoState* state )
{
    const SoTextureComposerElement* elem = (SoTextureComposerElement*)
	getConstElement(state,classStackIndex);

    return elem->units_;
}
