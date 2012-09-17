/*
___________________________________________________________________

 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: SoInvisibleLineDragger.cc,v 1.2 2009/07/22 16:01:35 cvsbert Exp $";


#include "SoInvisibleLineDragger.h"

#include "Inventor/SbRotation.h"

#include "Inventor/actions/SoGLRenderAction.h"

#include "Inventor/elements/SoCacheElement.h"
#include "Inventor/elements/SoModelMatrixElement.h"
#include "Inventor/elements/SoViewportRegionElement.h"
#include "Inventor/elements/SoViewVolumeElement.h"

#include "Inventor/events/SoKeyboardEvent.h"

#include "Inventor/nodes/SoCoordinate3.h"
#include "SoForegroundTranslation.h"
#include "Inventor/nodes/SoIndexedFaceSet.h"
#include "Inventor/nodes/SoMaterial.h"
#include "Inventor/nodes/SoMaterialBinding.h"
#include "Inventor/nodes/SoNormal.h"
#include "Inventor/nodes/SoNormalBinding.h"
#include "Inventor/nodes/SoPolygonOffset.h"
#include "Inventor/nodes/SoSeparator.h"
#include "Inventor/nodes/SoShapeHints.h"
#include "Inventor/nodes/SoSwitch.h"

#include <Inventor/projectors/SbLineProjector.h>
#include <Inventor/projectors/SbPlaneProjector.h>



#define WHATKIND_NONE      0
#define WHATKIND_SCALE     1
#define WHATKIND_LINETRANSLATE 2
#define WHATKIND_PLANETRANSLATE 3

#define CONSTRAINT_OFF  0
#define CONSTRAINT_WAIT 1
#define CONSTRAINT_X    2
#define CONSTRAINT_Y    3
#define CONSTRAINT_Z    4

#define Z_OFFSET 0.01f 
#define TABSIZE 10.0f 


SO_KIT_SOURCE(SoInvisibleLineDragger);

void SoInvisibleLineDragger::initClass()
{
    SO_KIT_INIT_CLASS(SoInvisibleLineDragger, SoDragger, "Dragger");
}


SoInvisibleLineDragger::SoInvisibleLineDragger()
    : lineProj_( 0 )
{
    SO_KIT_CONSTRUCTOR( SoInvisibleLineDragger );

    SO_KIT_ADD_CATALOG_ENTRY(shape, SoNode, true,
			    geomSeparator, "", true);

    SO_KIT_INIT_INSTANCE();

    addStartCallback(SoInvisibleLineDragger::startCB);
    addMotionCallback(SoInvisibleLineDragger::motionCB);

    setUpConnections(true, true);
}


SoInvisibleLineDragger::~SoInvisibleLineDragger()
{
    delete lineProj_;
}


void SoInvisibleLineDragger::setDirection( const SbVec3f& dir )
{
    if ( !lineProj_ ) lineProj_ = new SbLineProjector;
    lineProj_->setLine( SbLine(startPos, startPos + dir ) );
}


void SoInvisibleLineDragger::dragStart(void)
{
    const SoPath* pickpath = getPickPath();
    const SoEvent* event = getEvent();

    startPos = getLocalStartingPoint();
    translation = SbVec3f( 0, 0, 0 );
    needsDirection.invokeCallbacks( this );
}


void SoInvisibleLineDragger::drag(void)
{
    if ( !lineProj_ )
	return;

    SbMatrix motmat;

    const SbVec3f startpt = getLocalStartingPoint();

    lineProj_->setViewVolume(getViewVolume());
    lineProj_->setWorkingSpace(getLocalToWorldMatrix());
    const SbVec3f newhitpt = lineProj_->project(getNormalizedLocaterPosition());

    translation = newhitpt-startpt;
}


void SoInvisibleLineDragger::startCB(void* , SoDragger* d)
{
    SoInvisibleLineDragger* thisp = (SoInvisibleLineDragger*)d;
    thisp->dragStart();
}


void SoInvisibleLineDragger::motionCB(void*, SoDragger* d)
{
    SoInvisibleLineDragger* thisp = (SoInvisibleLineDragger*)d;
    thisp->drag();
}
