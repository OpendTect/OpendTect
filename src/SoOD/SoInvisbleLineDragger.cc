/*
___________________________________________________________________

 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jul 2003
___________________________________________________________________

-*/

static const char* rcsID = "$Id: SoInvisbleLineDragger.cc,v 1.1 2007-10-18 13:52:04 cvskris Exp $";


#include "SoInvisbleLineDragger.h"

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


SO_KIT_SOURCE(SoInvisbleLineDragger);

void SoInvisbleLineDragger::initClass()
{
    SO_KIT_INIT_CLASS(SoInvisbleLineDragger, SoDragger, "Dragger");
}


SoInvisbleLineDragger::SoInvisbleLineDragger()
    : lineProj_( 0 )
{
    SO_KIT_CONSTRUCTOR( SoInvisbleLineDragger );

    SO_KIT_ADD_CATALOG_ENTRY(shape, SoNode, true,
			    geomSeparator, "", true);

    SO_KIT_INIT_INSTANCE();

    addStartCallback(SoInvisbleLineDragger::startCB);
    addMotionCallback(SoInvisbleLineDragger::motionCB);

    setUpConnections(true, true);
}


SoInvisbleLineDragger::~SoInvisbleLineDragger()
{
    delete lineProj_;
}


void SoInvisbleLineDragger::setDirection( const SbVec3f& dir )
{
    if ( !lineProj_ ) lineProj_ = new SbLineProjector;
    lineProj_->setLine( SbLine(startPos, startPos + dir ) );
}


void SoInvisbleLineDragger::dragStart(void)
{
    const SoPath* pickpath = getPickPath();
    const SoEvent* event = getEvent();

    startPos = getLocalStartingPoint();
    translation = SbVec3f( 0, 0, 0 );
    needsDirection.invokeCallbacks( this );
}


void SoInvisbleLineDragger::drag(void)
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


void SoInvisbleLineDragger::startCB(void* , SoDragger* d)
{
    SoInvisbleLineDragger* thisp = (SoInvisbleLineDragger*)d;
    thisp->dragStart();
}


void SoInvisbleLineDragger::motionCB(void*, SoDragger* d)
{
    SoInvisbleLineDragger* thisp = (SoInvisbleLineDragger*)d;
    thisp->drag();
}
