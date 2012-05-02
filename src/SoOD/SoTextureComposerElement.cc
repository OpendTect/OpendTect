/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          November 2007
________________________________________________________________________

-*/
static const char* mUnusedVar rcsID = "$Id: SoTextureComposerElement.cc,v 1.6 2012-05-02 11:53:27 cvskris Exp $";

#include "SoTextureComposerElement.h"

SO_ELEMENT_SOURCE( SoTextureComposerElement );

void SoTextureComposerElement::initClass()
{
    SO_ELEMENT_INIT_CLASS( SoTextureComposerElement, SoReplacedElement );
}


SoTextureComposerElement::~SoTextureComposerElement()
{}


void SoTextureComposerElement::set( SoState* const state, SoNode* node,
       				    const SbList<int>& units, char ti )
{
    SoTextureComposerElement * elem = (SoTextureComposerElement *)
	getElement( state, classStackIndex, node );

    if ( elem ) elem->setElt( units, ti );
}


void SoTextureComposerElement::setElt( const SbList<int>& units, char ti )
{
    units_ = units;
    ti_ = ti;
}


const SbList<int>& SoTextureComposerElement::getUnits( SoState* state )
{
    const SoTextureComposerElement* elem = (SoTextureComposerElement*)
	getConstElement(state,classStackIndex);

    return elem->units_;
}


char
SoTextureComposerElement::getTransparencyInfo( SoState* state )
{
    const SoTextureComposerElement* elem = (SoTextureComposerElement*)
	    getConstElement(state,classStackIndex);

    return elem->ti_;
}
