/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: SoShapeScale.cc,v 1.7 2004-05-11 12:44:31 kristofer Exp $";


#include "SoShapeScale.h"

#include <Inventor/SbLinear.h>
#include <Inventor/actions/SoAction.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>

SO_NODE_SOURCE(SoShapeScale);

SoShapeScale::SoShapeScale(void) 
{
    SO_NODE_CONSTRUCTOR(SoShapeScale);
    SO_NODE_ADD_FIELD(restoreProportions, (true));
    SO_NODE_ADD_FIELD(dorotate, (false));
    SO_NODE_ADD_FIELD(screenSize, (5));
}


SoShapeScale::~SoShapeScale()
{ }


void SoShapeScale::initClass(void)
{
    SO_NODE_INIT_CLASS(SoShapeScale, SoNode, "Node");
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
mImplAction( getPrimitiveCount, SoGetPrimitiveCountAction )

void SoShapeScale::doAction( SoAction* action )
{
    SoState* state = action->getState();
    
    const SbMatrix& mat = SoModelMatrixElement::get(state);
    const SbViewVolume& vv = SoViewVolumeElement::get(state);

    SbVec3f scaleby(1,1,1);
    bool changescale = false;
    if ( screenSize.getValue() )
    {
	SbVec3f worldcenter;
	mat.multVecMatrix(SbVec3f(0,0,0), worldcenter);

	const SbViewportRegion& vp = SoViewportRegionElement::get(state);
	
	const float nsize = screenSize.getValue()/
	    		float(vp.getViewportSizePixels()[1]);

	float scalefactor = vv.getWorldToScreenScale(worldcenter, nsize);
	scaleby = scaleby * scalefactor;
	changescale = true;
    }
    
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
	changescale = true;
    }

    if ( changescale )
	SoModelMatrixElement::scaleBy(state, this, scaleby);

}
