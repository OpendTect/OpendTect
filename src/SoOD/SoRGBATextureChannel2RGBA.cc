/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          September 2008
________________________________________________________________________

-*/
static const char* rcsID mUnusedVar = "$Id$";


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
    , ti_( SoTextureComposerInfo::cHasNoTransparency() )
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

    const int curnodeid = getNodeId();

    const bool needsregeneration = prevnodeid_!=curnodeid ||
	!matchinfo_ || !elem || !elem->matches( matchinfo_ );

    if ( needsregeneration )
    {
	const int nrinputchannels =
	    SoTextureChannelSetElement::getNrChannels( state );
	const SbImagei32* inputchannels =
	    SoTextureChannelSetElement::getChannels( state );

	for ( int idx=0; idx<4; idx++ )
	{
	    const bool isenab = idx<enabled.getNum() && idx<nrinputchannels
		? enabled[idx]
		: false;

	    SbVec3i32 size;
	    int nc;
	    unsigned const char* data = idx<nrinputchannels
		? inputchannels[idx].getValue( size, nc )
		: 0;

	    rgba_[idx].setValuePtr( size, nc, isenab ? data : 0 );

	    if ( idx!=3 )
		continue;

	    const int fullsize = size[0]*size[1]*size[2];
	    if ( !isenab || nc!=1 || !fullsize )
	    {
		ti_ = SoTextureComposerInfo::cHasNoTransparency();
		continue;
	    }

	    bool fullyopaque;
	    bool fullytransparent;
	    if ( data )
	    {
		fullyopaque = true;
		fullytransparent = true;
		for ( int idy=0; idy<fullsize; idy++ )
		{
		    if ( data[idy] )
		    {
			fullyopaque = false;
			if ( data[idy]!=255 )
			    fullytransparent = false;
		    }

		    if ( !fullytransparent && !fullyopaque )
			break;
		}
	    }
	    else
	    {
		fullyopaque = true;
		fullytransparent = false;
	    }

	    if ( !fullytransparent && !fullyopaque )
		ti_ = SoTextureComposerInfo::cHasTransparency();
	    else if ( fullyopaque )
		ti_ = SoTextureComposerInfo::cHasNoTransparency();
	    else
		ti_ = SoTextureComposerInfo::cHasNoIntermediateTransparency();
	}

	delete matchinfo_; matchinfo_ = elem->copyMatchInfo();
	prevnodeid_ = curnodeid;
    }

    SbList<uint32_t> dep;
    dep.append( elem->getNodeId() );
    SoTextureChannelSetElement::set( state, this, rgba_, 4, &dep );

    SbList<int> units;
    units.append( 0 );
    SoTextureComposerElement::set( state, this, units, ti_ );
}
