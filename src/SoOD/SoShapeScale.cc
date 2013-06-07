/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id$";


#include "SoShapeScale.h"

#include <Inventor/SbLinear.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>

SO_NODE_SOURCE(SoShapeScale);

SoShapeScale::SoShapeScale(void) 
    : scaleby( 1, 1, 1 )
    , changescale( -1 )
{
    SO_NODE_CONSTRUCTOR(SoShapeScale);
    SO_NODE_ADD_FIELD(restoreProportions, (true));
    SO_NODE_ADD_FIELD(dorotate, (false));
    SO_NODE_ADD_FIELD(screenSize, (5));
    SO_NODE_ADD_FIELD(minScale, (0));
    SO_NODE_ADD_FIELD(maxScale, (1e30));
}


SoShapeScale::~SoShapeScale()
{ }


void SoShapeScale::initClass(void)
{
    SO_NODE_INIT_CLASS(SoShapeScale, SoNode, "Node");
    SO_ENABLE(SoCallbackAction, SoModelMatrixElement );
    SO_ENABLE(SoGLRenderAction, SoModelMatrixElement );
    SO_ENABLE(SoGetBoundingBoxAction, SoModelMatrixElement );
    SO_ENABLE(SoPickAction, SoModelMatrixElement );
    SO_ENABLE(SoRayPickAction, SoModelMatrixElement );
    SO_ENABLE(SoGetPrimitiveCountAction, SoModelMatrixElement );
}

#define mImplAction( fn, act ) \
void SoShapeScale::fn(act* action ) \
{ \
    SoShapeScale::doAction( (SoAction*) action ); \
} \

mImplAction( GLRender, SoGLRenderAction )
mImplAction( callback, SoCallbackAction )
mImplAction( getBoundingBox, SoGetBoundingBoxAction )
mImplAction( pick, SoPickAction )
mImplAction( rayPick, SoRayPickAction )
mImplAction( getPrimitiveCount, SoGetPrimitiveCountAction )

void SoShapeScale::doAction( SoAction* action )
{
    SoState* state = action->getState();
    if (changescale==-1 ||
	    action->getTypeId()==SoGLRenderAction::getClassTypeId()) 
    {
	changescale = 0;
	const SbMatrix& mat = SoModelMatrixElement::get(state);
	const SbViewVolume& vv = SoViewVolumeElement::get(state);

	scaleby = SbVec3f(1,1,1);
	if ( screenSize.getValue() )
	{
	    SbVec3f worldcenter;
	    mat.multVecMatrix(SbVec3f(0,0,0), worldcenter);

	    const SbViewportRegion& vp = SoViewportRegionElement::get(state);
	    
	    const float nsize = screenSize.getValue()/
			    float(vp.getViewportSizePixels()[1]);

	    float scalefactor = vv.getWorldToScreenScale(worldcenter, nsize);
	    scaleby = scaleby * scalefactor;
	    changescale = 1;
	}

	const float minscale = minScale.getValue();
	const float maxscale = maxScale.getValue();
	if ( scaleby[0]<minscale )
	    scaleby = SbVec3f(minscale,minscale,minscale);
	else if ( scaleby[0]>maxscale )
	    scaleby = SbVec3f(maxscale,maxscale,maxscale);

	if ( restoreProportions.getValue() )
	{
	    SbVec3f dummmyt;
	    SbRotation dummyr;
	    SbVec3f scale;
	    SbRotation dummyr2;
	    mat.getTransform (dummmyt, dummyr, scale, dummyr2 );
	    scaleby[0] /= scale[0];
	    scaleby[1] /= scale[1];
	    scaleby[2] /= scale[2];
	    changescale = 1;
	}
    }

    if ( changescale==1 )
	SoModelMatrixElement::scaleBy(state, this, scaleby);
}
