/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID = "$Id: SoShapeScale.cc,v 1.3 2003-11-07 12:22:02 bert Exp $";


#include "SoShapeScale.h"

#include "gendefs.h"
#include "trigonometry.h"

#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/nodes/SoShape.h>
#include <Inventor/nodes/SoScale.h>
#include <Inventor/nodes/SoRotation.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoViewportRegionElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>

SO_KIT_SOURCE(SoShapeScale);

SoShapeScale::SoShapeScale(void) 
{
    SO_KIT_CONSTRUCTOR(SoShapeScale);
    SO_KIT_ADD_FIELD(doscale, (TRUE));
    SO_KIT_ADD_FIELD(dorotate, (TRUE));
    SO_KIT_ADD_FIELD(projectedSize, (5.0f));
    
    SO_KIT_ADD_CATALOG_ENTRY(topSeparator, SoSeparator, FALSE, this, "", FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(rotation, SoRotation, FALSE, topSeparator,
	    		     scale, FALSE);
    SO_KIT_ADD_CATALOG_ENTRY(scale, SoScale, FALSE, topSeparator, shape, FALSE);
    SO_KIT_ADD_CATALOG_ABSTRACT_ENTRY(shape, SoNode, SoCube, TRUE, topSeparator,
	   			      "", TRUE);

    SO_KIT_INIT_INSTANCE();
}


SoShapeScale::~SoShapeScale()
{ }


void SoShapeScale::initClass(void)
{
    static int first = 1;
    if (first)
    {
	first = 0;
	SO_KIT_INIT_CLASS(SoShapeScale, SoBaseKit, "BaseKit");
    }
}


static void update_scale(SoScale* scale, const SbVec3f & v)
{
    if (scale->scaleFactor.getValue() != v)
    {
	bool oldnotify = scale->enableNotify( false );
	scale->scaleFactor = v;
	oldnotify = scale->enableNotify( oldnotify );
    }
}


void SoShapeScale::GLRender(SoGLRenderAction * action)
{
    if ( !doscale.getValue() && !dorotate.getValue() )
	return inherited::GLRender(action);

    SoState* state = action->getState();
    
    SoScale* scalenode = (SoScale*) getAnyPart(SbName("scale"), true);
    SoRotation* rotnode = (SoRotation*) getAnyPart(SbName("rotation"), true);

    const SbMatrix &mat = SoModelMatrixElement::get(state);
    const SbViewVolume & vv = SoViewVolumeElement::get(state);
    const SbVec3f localcenter(0.0f, 0.0f, 0.0f);
    SbVec3f worldcenter;
    mat.multVecMatrix(localcenter, worldcenter);


    if ( doscale.getValue() )
    {
	const SbViewportRegion & vp = SoViewportRegionElement::get(state);
	float nsize = projectedSize.getValue() /
	    		float(vp.getViewportSizePixels()[1]);

	float scalefactor = vv.getWorldToScreenScale(worldcenter, nsize);
	update_scale(scalenode, SbVec3f(scalefactor, scalefactor, scalefactor));
    }

    if ( dorotate.getValue() )
    {
	SbVec3f tovec = (vv.getProjectionPoint() - worldcenter);
	SbVec3f localfrom( 0, 1, 0 );
	SbVec3f worldfrom;
	mat.multVecMatrix(localfrom, worldfrom);
	Vector3 worldfromvec( worldfrom[0], worldfrom[1],  worldfrom[2] );
	Vector3 worldtovec( tovec[0], tovec[1],  tovec[2] );
	Vector3 rotationaxis = worldfromvec.cross( worldtovec );
	float angle = acos(worldfromvec.dot( worldtovec ));

	if ( mIS_ZERO( rotationaxis.abs() ) )
	{
	    rotationaxis.x = 1;
	}

	SbRotation rotation(
		SbVec3f(rotationaxis.x,rotationaxis.y,rotationaxis.z), -angle );

	bool oldnotify = rotnode->enableNotify( false );
	rotnode->rotation.setValue( rotation );
	rotnode->enableNotify( oldnotify );
    }

    inherited::GLRender(action);
}

    
	

