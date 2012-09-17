/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: SoCameraInfo.cc,v 1.4 2009/07/22 16:01:35 cvsbert Exp $";

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
    
    SO_NODE_DEFINE_ENUM_VALUE(CameraStatus, NORMAL );
    SO_NODE_DEFINE_ENUM_VALUE(CameraStatus, MOVING );
    SO_NODE_DEFINE_ENUM_VALUE(CameraStatus, INTERACTIVE);
    SO_NODE_DEFINE_ENUM_VALUE(CameraStatus, STEREO );
}


void SoCameraInfo::GLRender(SoGLRenderAction* action)
{
    if ( cameraInfo.isIgnored() )
	return;

    SoCameraInfoElement::set( action->getState(), this, cameraInfo.getValue() );
}
