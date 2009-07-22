/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          December 2006
________________________________________________________________________

-*/
static const char* rcsID = "$Id: SoTextureChannelSet.cc,v 1.3 2009-07-22 16:01:35 cvsbert Exp $";


#include "SoTextureChannelSet.h"

#include <SoTextureChannelSetElement.h>
#include "Inventor/actions/SoGLRenderAction.h"


SO_NODE_SOURCE( SoTextureChannelSet );

void SoTextureChannelSet::initClass()
{
    SO_NODE_INIT_CLASS(SoTextureChannelSet, SoNode, "Node");
    SO_ENABLE(SoGLRenderAction, SoTextureChannelSetElement);
}


SoTextureChannelSet::SoTextureChannelSet()
{
    SO_NODE_CONSTRUCTOR( SoTextureChannelSet );
    SO_NODE_ADD_FIELD( channels, (SbImage(0, SbVec2s(0,0), 0)) );
}


SoTextureChannelSet::~SoTextureChannelSet()
{}


void SoTextureChannelSet::GLRender( SoGLRenderAction* action )
{
    SoState* state = action->getState();

    SoTextureChannelSetElement::set( state, this, channels.getValues(0),
	    			  channels.getNum() );
}
