/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: SoShapeScale.cc,v 1.1 2002-07-12 07:31:41 kristofer Exp $";


#include "SoShapeScale.h"

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/nodes/SoShape.h>
#include <Inventor/nodes/SoScale.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>

SO_KIT_SOURCE(SoShapeScale);

SoShapeScale::SoShapeScale(void) 
{
    SO_KIT_CONSTRUCTOR(SoShapeScale);
    SO_KIT_ADD_FIELD(active, (TRUE));
    SO_KIT_ADD_FIELD(projectedSize, (5.0f));
    
    SO_KIT_ADD_CATALOG_ENTRY(topSeparator, SoSeparator, FALSE, this, "", FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(scale, SoScale, FALSE, topSeparator, shape, FALSE);
    SO_KIT_ADD_CATALOG_ABSTRACT_ENTRY(shape, SoNode, SoCube, TRUE, topSeparator,
	   			      "", TRUE);

    SO_KIT_INIT_INSTANCE();
}


SoShapeScale::~SoShapeScale()
{
}


void SoShapeScale::initClass(void)
{
    static int first = 1;
    if (first)
    {
	first = 0;
	SO_KIT_INIT_CLASS(SoShapeScale, SoBaseKit, "BaseKit");
    }
}


static void update_scale(SoScale * scale, const SbVec3f & v)
{
    if (scale->scaleFactor.getValue() != v) { scale->scaleFactor = v; }
}


void SoShapeScale::GLRender(SoGLRenderAction * action)
{
    SoState * state = action->getState();
    
    SoScale * scale = (SoScale*) this->getAnyPart(SbName("scale"), TRUE);
    if (!this->active.getValue())
    {
	update_scale(scale, SbVec3f(1.0f, 1.0f, 1.0f));
    }
    else
    {
	const SbViewportRegion & vp = SoViewportRegionElement::get(state);
	const SbViewVolume & vv = SoViewVolumeElement::get(state);
	SbVec3f center(0.0f, 0.0f, 0.0f);
	float nsize = this->projectedSize.getValue() /
	    		float(vp.getViewportSizePixels()[1]);
	SoModelMatrixElement::get(state).multVecMatrix(center, center);
	float scalefactor = vv.getWorldToScreenScale(center, nsize);
	update_scale(scale, SbVec3f(scalefactor, scalefactor, scalefactor));
    }

    inherited::GLRender(action);
}

    
	

