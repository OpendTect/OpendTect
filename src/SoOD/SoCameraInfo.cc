/*
___________________________________________________________________

 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: SoCameraInfo.cc,v 1.1 2003-09-02 12:36:03 kristofer Exp $";

#include "SoCameraInfo.h"

#include "SoCameraInfoElement.h"

#include "Inventor/actions/SoGLRenderAction.h"

SO_NODE_SOURCE(SoCameraInfo);

void SoCameraInfo::initClass()
{
    SO_NODE_INIT_CLASS(SoCameraInfo, SoNode, "Node" );
    SO_ENABLE(SoGLRenderAction, SoCameraInfoElement);
}


SoCameraInfo::SoCameraInfo()
{
    SO_NODE_CONSTRUCTOR(SoCameraInfo);

    SO_NODE_ADD_FIELD( cameraInfo, (SoCameraInfo::NORMAL) );
    
    SO_NODE_DEFINE_ENUM_VALUE(cameraStatus, NORMAL );
    SO_NODE_DEFINE_ENUM_VALUE(cameraStatus, MOVING );
    SO_NODE_DEFINE_ENUM_VALUE(cameraStatus, INTERACTIVE);
    SO_NODE_DEFINE_ENUM_VALUE(cameraStatus, STEREO );
}


void SoCameraInfo::GLRender(SoGLRenderAction* action)
{
    if ( cameraInfo.isIgnored() )
	return;

    SoCameraInfoElement::set( action->getState(), this, cameraInfo.getValue() );
}
