/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          November 2007
 RCS:           $Id: SoTextureComposerElement.cc,v 1.2 2008-10-21 21:09:55 cvskris Exp $
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
       				    const SbList<int>& units,
				    SoTextureComposer::ForceTransparency ft )
{
    SoTextureComposerElement * elem = (SoTextureComposerElement *)
	getElement( state, classStackIndex, node );

    if ( elem ) elem->setElt( units, ft );
}


void SoTextureComposerElement::setElt( const SbList<int>& units,
				       SoTextureComposer::ForceTransparency ft )
{
    units_ = units;
    ft_ = ft;
}


const SbList<int>& SoTextureComposerElement::getUnits( SoState* state )
{
    const SoTextureComposerElement* elem = (SoTextureComposerElement*)
	getConstElement(state,classStackIndex);

    return elem->units_;
}


SoTextureComposer::ForceTransparency
SoTextureComposerElement::getForceTrans( SoState* state )
{
    const SoTextureComposerElement* elem = (SoTextureComposerElement*)
	    getConstElement(state,classStackIndex);

    return elem->ft_;
}
