/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: SoShapeScale.cc,v 1.6 2004-05-11 12:17:49 kristofer Exp $";


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
    SO_NODE_ADD_FIELD(doscale, (true));
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

    if ( doscale.getValue() )
    {
	SbVec3f worldcenter;
	mat.multVecMatrix(SbVec3f(0,0,0), worldcenter);

	const SbViewportRegion& vp = SoViewportRegionElement::get(state);
	
	const float nsize = screenSize.getValue()/
	    		float(vp.getViewportSizePixels()[1]);

	SbVec3f dummmyt;
	SbRotation dummyr;
	SbVec3f scale;
	SbRotation dummyr2;
	mat.getTransform (dummmyt, dummyr, scale, dummyr2 );

	const SbVec3f invscale(1/scale[0], 1/scale[1], 1/scale[2]);

	float scalefactor = vv.getWorldToScreenScale(worldcenter, nsize);
	const SbVec3f newscale( invscale*scalefactor );
	SoModelMatrixElement::scaleBy(state, this, newscale);
    }
}
