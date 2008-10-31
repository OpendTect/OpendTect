/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          September 2008
 RCS:           $Id: SoRGBATextureChannel2RGBA.cc,v 1.1 2008-10-31 18:28:49 cvskris Exp $
________________________________________________________________________

-*/


#include "SoRGBATextureChannel2RGBA.h"

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
{
    SO_NODE_CONSTRUCTOR( SoRGBATextureChannel2RGBA );
    SO_NODE_ADD_FIELD( enabled, (true) );
}


SoRGBATextureChannel2RGBA::~SoRGBATextureChannel2RGBA()
{}


void SoRGBATextureChannel2RGBA::GLRender( SoGLRenderAction* action )
{
    SoState* state = action->getState();
    const int nrinputchannels =
	SoTextureChannelSetElement::getNrChannels( state );
    const SbImage* inputchannels =
	SoTextureChannelSetElement::getChannels( state );

    bool ismissing = false;
    for ( int idx=0; idx<4; idx++ )
    {
	const bool isenab = idx<enabled.getNum() && idx<nrinputchannels
	    ? enabled[idx]
	    : false;

	SbVec3s size;
	int nc;
	unsigned const char* data = inputchannels[idx].getValue( size, nc );

	rgba_[idx].setValuePtr( size, nc, isenab ? data : 0 );
	if ( !isenab )
	    ismissing = true;
    }

    if ( ismissing )
	SoTextureChannelSetElement::set( state, this, rgba_, 4 );

    SbList<int> units;
    units.append( 0 );
    SoTextureComposerElement::set( state, this, units, 0 );
}
