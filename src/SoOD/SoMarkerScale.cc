/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: SoMarkerScale.cc,v 1.2 2003-11-07 12:22:02 bert Exp $";


#include "SoMarkerScale.h"


#include <Inventor/actions/SoAction.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>

SO_NODE_SOURCE(SoMarkerScale);

SoMarkerScale::SoMarkerScale(void) 
{
    SO_NODE_CONSTRUCTOR(SoMarkerScale);
    SO_NODE_ADD_FIELD(translation, (0,0,0));
    SO_NODE_ADD_FIELD(scaleFactor, (1,1,1));
    SO_NODE_ADD_FIELD(screenSize, (5));
}


SoMarkerScale::~SoMarkerScale()
{ }


void SoMarkerScale::initClass(void)
{
    SO_NODE_INIT_CLASS(SoMarkerScale, SoNode, "Node");
}

#define mImplAction( fn, act ) \
void SoMarkerScale::fn(act* action ) \
{ \
    SoMarkerScale::doAction( (SoAction*) action ); \
} \

mImplAction( GLRender, SoGLRenderAction )
mImplAction( callback, SoCallbackAction )
mImplAction( getBoundingBox, SoGetBoundingBoxAction )
mImplAction( pick, SoPickAction )
mImplAction( getPrimitiveCount, SoGetPrimitiveCountAction )

void SoMarkerScale::doAction( SoAction* action )
{
    SoState* state = action->getState();
    
    const SbMatrix& mat = SoModelMatrixElement::get(state);
    const SbViewVolume& vv = SoViewVolumeElement::get(state);
    SbVec3f worldcenter;
    mat.multVecMatrix(translation.getValue(), worldcenter);

    const SbViewportRegion& vp = SoViewportRegionElement::get(state);
    
    float nsize = screenSize.getValue() / float(vp.getViewportSizePixels()[1]);

    float scalefactor = vv.getWorldToScreenScale(worldcenter, nsize);
    const SbVec3f newscale( scaleFactor.getValue()*scalefactor );
    SoModelMatrixElement::translateBy(state, this, translation.getValue());
    SoModelMatrixElement::scaleBy(state, this, newscale);
}
