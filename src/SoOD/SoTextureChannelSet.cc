/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          December 2006
 RCS:           $Id: SoTextureChannelSet.cc,v 1.1 2008-09-16 16:17:01 cvskris Exp $
________________________________________________________________________

-*/


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
