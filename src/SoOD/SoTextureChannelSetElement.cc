/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          November 2007
 RCS:           $Id: SoTextureChannelSetElement.cc,v 1.1 2008-09-16 16:17:01 cvskris Exp $
________________________________________________________________________

-*/

#include "SoTextureChannelSetElement.h"

SO_ELEMENT_SOURCE( SoTextureChannelSetElement );

void SoTextureChannelSetElement::initClass()
{
    SO_ELEMENT_INIT_CLASS( SoTextureChannelSetElement, SoReplacedElement );
}


SoTextureChannelSetElement::~SoTextureChannelSetElement()
{}


void SoTextureChannelSetElement::set( SoState* const state, SoNode* node,
				   const SbImage* channels,
       				   int nrchannels)
{
    SoTextureChannelSetElement * elem = (SoTextureChannelSetElement *)
	getElement( state, classStackIndex, node );

    if ( elem )
	elem->setElt( channels, nrchannels );
}


void SoTextureChannelSetElement::setElt( const SbImage* channels,
				      int nrchannels )
{
    channels_ = channels;
    nrchannels_ = nrchannels;
}


int SoTextureChannelSetElement::getNrChannels( SoState* state )
{
    const SoTextureChannelSetElement* elem = (SoTextureChannelSetElement*)
	getConstElement(state,classStackIndex);

    return elem->nrchannels_;
}


const SbImage* SoTextureChannelSetElement::getChannels( SoState* state )
{
    const SoTextureChannelSetElement* elem = (SoTextureChannelSetElement*)
	getConstElement(state,classStackIndex);

    return elem->channels_;
}
