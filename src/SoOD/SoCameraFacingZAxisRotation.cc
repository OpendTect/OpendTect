/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

static const char* rcsID mUsedVar = "$Id$";


#include "SoCameraFacingZAxisRotation.h"

#include <Inventor/actions/SoGetMatrixAction.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>

SO_NODE_SOURCE(SoCameraFacingZAxisRotation);

void SoCameraFacingZAxisRotation::initClass()
{
    SO_NODE_INIT_CLASS( SoCameraFacingZAxisRotation, SoTransformation,
	    		"Transformation");
}


SoCameraFacingZAxisRotation::SoCameraFacingZAxisRotation()
    : currot_( SbVec3f(0,0,1), 0 )
{
    SO_NODE_CONSTRUCTOR(SoCameraFacingZAxisRotation);
    SO_NODE_ADD_FIELD( lock, (false) );
}


SoCameraFacingZAxisRotation::~SoCameraFacingZAxisRotation()
{ }


void SoCameraFacingZAxisRotation::doAction( SoAction* action )
{
    SoModelMatrixElement::rotateBy(action->getState(), this, currot_ );
    SoState* state = ((SoAction*)action)->getState();
    SoCacheElement::invalidate(state);
}


void SoCameraFacingZAxisRotation::callback(SoCallbackAction* action)
{
    SoCameraFacingZAxisRotation::doAction((SoAction*)action);
}


void SoCameraFacingZAxisRotation::GLRender(SoGLRenderAction* action)
{
    if ( !lock.getValue() )
    {
	SoState* state = ((SoAction*)action)->getState();
	const SbViewVolume& vv = SoViewVolumeElement::get(state);
	const SbVec3f projectiondir = vv.getProjectionDirection();
	if ( fabs(projectiondir[2])<0.98 )
	{
	    const float angle = atan2(projectiondir[0],projectiondir[1]);
	    currot_.setValue( SbVec3f(0,0,-1), angle );
	}
    }

    SoCameraFacingZAxisRotation::doAction((SoAction*)action);
}


void SoCameraFacingZAxisRotation::getBoundingBox(SoGetBoundingBoxAction* action)
{
    SoCameraFacingZAxisRotation::doAction((SoAction*)action);
}


void SoCameraFacingZAxisRotation::getMatrix(SoGetMatrixAction* action)
{
    SbMatrix m;

    currot_.getValue(m);
    action->getMatrix().multLeft(m);

    SbRotation ri = currot_.inverse();
    ri.getValue(m);
    action->getInverse().multRight(m);
}


void SoCameraFacingZAxisRotation::pick(SoPickAction* action)
{
    SoCameraFacingZAxisRotation::doAction((SoAction*)action);
}


void SoCameraFacingZAxisRotation::getPrimitiveCount(
	SoGetPrimitiveCountAction* action)
{
      SoCameraFacingZAxisRotation::doAction((SoAction*)action);
}

