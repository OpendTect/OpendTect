/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Kristofer Tingdahl
 Date:          November 2007
 RCS:           $Id: SoTextureChannelSetElement.cc,v 1.2 2008-11-20 16:21:53 cvskris Exp $
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
       				   int nrchannels,
				   const SbList<uint32_t>* additionalnodeids)
{
    SoTextureChannelSetElement * elem = (SoTextureChannelSetElement *)
	getElement( state, classStackIndex, node, additionalnodeids );

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


SoElement* SoTextureChannelSetElement::copyMatchInfo(void) const
{
    SoTextureChannelSetElement* element =
	new SoTextureChannelSetElement;

    element->nodeId = nodeId;
    element->additionalnodeids_ = additionalnodeids_;
    return element;
}


SbBool SoTextureChannelSetElement::matches( const SoElement* element ) const
{
    if ( !SoReplacedElement::matches( element ) )
	return false;

    for ( int idx=additionalnodeids_.getLength()-1; idx>=0; idx-- )
    {
	if ( ((SoTextureChannelSetElement*)element)->additionalnodeids_.find(
		    additionalnodeids_[idx] )==-1 )
	    return false;
    }

    return true;
}



SoElement* SoTextureChannelSetElement::getElement(SoState* const state,
	const int stackIndex, SoNode* const node,
	const SbList<uint32_t>* additionalnodeids )
{
    SoTextureChannelSetElement* elem = (SoTextureChannelSetElement*)
	SoReplacedElement::getElement( state, stackIndex, node );

    if ( additionalnodeids )
	elem->additionalnodeids_ = *additionalnodeids;
    else
	elem->additionalnodeids_.truncate( 0 );

    return elem;
}
