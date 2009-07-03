/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          September 2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: SoRGBATextureChannel2RGBA.cc,v 1.5 2009-07-03 21:49:07 cvskris Exp $";


#include "SoRGBATextureChannel2RGBA.h"

#include "Inventor/elements/SoCacheElement.h"
#include "Inventor/actions/SoGLRenderAction.h"
#include "Inventor/fields/SoSFImage.h"
#include "SoTextureChannelSetElement.h"
#include "SoTextureComposerElement.h"

SO_NODE_SOURCE( SoRGBATextureChannel2RGBA );

void SoRGBATextureChannel2RGBA::initClass()
{
    SO_NODE_INIT_CLASS(SoRGBATextureChannel2RGBA, SoNode, "Node");

    SO_ENABLE(SoGLRenderAction, SoTextureChannelSetElement );
    SO_ENABLE(SoGLRenderAction, SoTextureComposerElement );
}


SoRGBATextureChannel2RGBA::SoRGBATextureChannel2RGBA()
    : matchinfo_( 0 )
    , prevnodeid_( -1 )
    , didsend_( false )
{
    SO_NODE_CONSTRUCTOR( SoRGBATextureChannel2RGBA );
    SO_NODE_ADD_FIELD( enabled, (true) );
}


SoRGBATextureChannel2RGBA::~SoRGBATextureChannel2RGBA()
{
    delete matchinfo_;
}


void SoRGBATextureChannel2RGBA::GLRender( SoGLRenderAction* action )
{
    SoState* state = action->getState();

    const SoTextureChannelSetElement* elem = (const SoTextureChannelSetElement*)
    state->getConstElement(SoTextureChannelSetElement::getClassStackIndex() );

    const bool needsregeneration = prevnodeid_!=getNodeId() ||
	!matchinfo_ || !elem || !elem->matches( matchinfo_ );

    if ( needsregeneration )
    {
	didsend_ = false;

	const int nrinputchannels =
	    SoTextureChannelSetElement::getNrChannels( state );
	const SbImage* inputchannels =
	    SoTextureChannelSetElement::getChannels( state );

	for ( int idx=0; idx<4; idx++ )
	{
	    const bool isenab = idx<enabled.getNum() && idx<nrinputchannels
		? enabled[idx]
		: false;

	    SbVec3s size;
	    int nc;
	    unsigned const char* data = idx<nrinputchannels
		? inputchannels[idx].getValue( size, nc )
		: 0;

	    rgba_[idx].setValuePtr( size, nc, isenab ? data : 0 );
	    if ( !isenab )
		didsend_ = true;
	}

	delete matchinfo_; matchinfo_ = elem->copyMatchInfo();
	touch(); //trigger nodeid to go up.
	prevnodeid_ = getNodeId();
    }

    if ( didsend_ )
    {
	SoTextureChannelSetElement::set( state, this, rgba_, 4 );
    }

    SbList<int> units;
    units.append( 0 );
    SoTextureComposerElement::set( state, this, units, 0 );
}
