/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Jan 2002
-*/

static const char* rcsID = "$Id: visrectangle.cc,v 1.41 2004-07-29 16:52:30 bert Exp $";

#include "visrectangle.h"
#include "iopar.h"
#include "survinfo.h"

#include <Inventor/nodes/SoScale.h>
#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoRotation.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoNormalBinding.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoGroup.h>

#include <Inventor/draggers/SoTabPlaneDragger.h>
#include <Inventor/draggers/SoTranslate1Dragger.h>

#include <math.h>

mCreateFactoryEntry( visBase::RectangleDragger );

visBase::RectangleDragger::RectangleDragger()
    : started( this )
    , motion( this )
    , changed( this )
    , finished( this )
    , root( new SoSeparator )
    , manipzdraggertop( new SoTranslate1Dragger )
    , manipzdraggerright( new SoTranslate1Dragger )
    , manipzdraggerbottom( new SoTranslate1Dragger )
    , manipzdraggerleft( new SoTranslate1Dragger )
    , manipxydragger0( new SoTabPlaneDragger )
    , manipxydragger1( new SoTabPlaneDragger )
    , zdraggerscale( new SoScale )
    , tabswitch( new SoSwitch )
    , allowcb( true )
{
    SoSeparator* zmanipsep = new SoSeparator;
    root->addChild( zmanipsep );

    zmanipsep->addChild( zdraggerscale );

    SoRotation* rot = new SoRotation;
    zmanipsep->addChild( rot );
    rot->rotation.setValue( SbVec3f( 0,1,0), -M_PI/2 );

    zmanipsep->addChild( manipzdraggertop );
    manipzdraggertop->addStartCallback(
	    visBase::RectangleDragger::startCB, this );
    manipzdraggertop->addMotionCallback(
	    visBase::RectangleDragger::motionCB, this );
    manipzdraggertop->addValueChangedCallback(
	    visBase::RectangleDragger::valueChangedCB, this );
    manipzdraggertop->addFinishCallback(
	    visBase::RectangleDragger::finishCB, this );

    zmanipsep->addChild( manipzdraggerright );
    manipzdraggerright->addStartCallback(
	    visBase::RectangleDragger::startCB, this );
    manipzdraggerright->addMotionCallback(
	    visBase::RectangleDragger::motionCB, this );
    manipzdraggerright->addValueChangedCallback(
	    visBase::RectangleDragger::valueChangedCB, this );
    manipzdraggerright->addFinishCallback(
	    visBase::RectangleDragger::finishCB, this );

    zmanipsep->addChild( manipzdraggerbottom );
    manipzdraggerbottom->addStartCallback(
	    visBase::RectangleDragger::startCB, this );
    manipzdraggerbottom->addMotionCallback(
	    visBase::RectangleDragger::motionCB, this );
    manipzdraggerbottom->addValueChangedCallback(
	    visBase::RectangleDragger::valueChangedCB, this );
    manipzdraggerbottom->addFinishCallback(
	    visBase::RectangleDragger::finishCB, this );

    zmanipsep->addChild( manipzdraggerleft );
    manipzdraggerleft->addStartCallback(
	    visBase::RectangleDragger::startCB, this );
    manipzdraggerleft->addMotionCallback(
	    visBase::RectangleDragger::motionCB, this );
    manipzdraggerleft->addValueChangedCallback(
	    visBase::RectangleDragger::valueChangedCB, this );
    manipzdraggerleft->addFinishCallback(
	    visBase::RectangleDragger::finishCB, this );

    root->addChild( tabswitch );
    SoGroup* tabgrp = new SoGroup;
    tabswitch->addChild( tabgrp );
    tabgrp->addChild( manipxydragger0 );
    manipxydragger0->addStartCallback(
	    visBase::RectangleDragger::startCB, this );
    manipxydragger0->addMotionCallback(
	    visBase::RectangleDragger::motionCB, this );
    manipxydragger0->addValueChangedCallback(
	    visBase::RectangleDragger::valueChangedCB, this );
    manipxydragger0->addFinishCallback(
	    visBase::RectangleDragger::finishCB, this );

    SoRotation* manipxydragger1rot = new SoRotation;
    tabgrp->addChild( manipxydragger1rot );
    manipxydragger1rot->rotation.setValue( SbVec3f( 1, 0, 0 ), M_PI );
    tabgrp->addChild( manipxydragger1 );
    manipxydragger1->addStartCallback(
	    visBase::RectangleDragger::startCB, this );
    manipxydragger1->addMotionCallback(
	    visBase::RectangleDragger::motionCB, this );
    manipxydragger1->addValueChangedCallback(
	    visBase::RectangleDragger::valueChangedCB, this );
    manipxydragger1->addFinishCallback(
	    visBase::RectangleDragger::finishCB, this );

    tabswitch->whichChild = 0;
    syncronizeDraggers();

    setOwnShapeHints();
}


visBase::RectangleDragger::~RectangleDragger()
{
    manipzdraggertop->removeStartCallback(
	    visBase::RectangleDragger::startCB, this );
    manipzdraggertop->removeMotionCallback(
	    visBase::RectangleDragger::motionCB, this );
    manipzdraggertop->removeValueChangedCallback(
	    visBase::RectangleDragger::valueChangedCB, this );
    manipzdraggertop->removeFinishCallback(
	    visBase::RectangleDragger::finishCB, this );

    manipzdraggerright->removeStartCallback(
	    visBase::RectangleDragger::startCB, this );
    manipzdraggerright->removeMotionCallback(
	    visBase::RectangleDragger::motionCB, this );
    manipzdraggerright->removeValueChangedCallback(
	    visBase::RectangleDragger::valueChangedCB, this );
    manipzdraggerright->removeFinishCallback(
	    visBase::RectangleDragger::finishCB, this );

    manipzdraggerbottom->removeStartCallback(
	    visBase::RectangleDragger::startCB, this );
    manipzdraggerbottom->removeMotionCallback(
	    visBase::RectangleDragger::motionCB, this );
    manipzdraggerbottom->removeValueChangedCallback(
	    visBase::RectangleDragger::valueChangedCB, this );
    manipzdraggerbottom->removeFinishCallback(
	    visBase::RectangleDragger::finishCB, this );

    manipzdraggerleft->removeStartCallback(
	    visBase::RectangleDragger::startCB, this );
    manipzdraggerleft->removeMotionCallback(
	    visBase::RectangleDragger::motionCB, this );
    manipzdraggerleft->removeValueChangedCallback(
	    visBase::RectangleDragger::valueChangedCB, this );
    manipzdraggerleft->removeFinishCallback(
	    visBase::RectangleDragger::finishCB, this );

    manipxydragger0->removeStartCallback(
	    visBase::RectangleDragger::startCB, this );
    manipxydragger0->removeMotionCallback(
	    visBase::RectangleDragger::motionCB, this );
    manipxydragger0->removeValueChangedCallback(
	    visBase::RectangleDragger::valueChangedCB, this );
    manipxydragger0->removeFinishCallback(
	    visBase::RectangleDragger::finishCB, this );

    manipxydragger1->removeStartCallback(
	    visBase::RectangleDragger::startCB, this );
    manipxydragger1->removeMotionCallback(
	    visBase::RectangleDragger::motionCB, this );
    manipxydragger1->removeValueChangedCallback(
	    visBase::RectangleDragger::valueChangedCB, this );
    manipxydragger1->removeFinishCallback(
	    visBase::RectangleDragger::finishCB, this );
}


void visBase::RectangleDragger::setOwnShapeHints()
{
    SoShapeHints* myHints = new SoShapeHints;
    myHints->shapeType = SoShapeHints::SOLID;
    myHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    myHints->vertexOrdering = SI().isClockWise()
	    ? SoShapeHints::COUNTERCLOCKWISE : SoShapeHints::CLOCKWISE;

    manipxydragger0->setPart( "scaleTabHints", myHints );
    manipxydragger1->setPart( "scaleTabHints", myHints );
}


void visBase::RectangleDragger::setCenter( const Coord3& pos_ )
{
    bool allowcb_bak = allowcb;
    allowcb = false;

    Coord3 pos( pos_ );

    manipxydragger0->translation.setValue( pos.x, pos.y, pos.z );

    float xd = manipzdraggertop->translation.getValue()[2] /
	       zdraggerscale->scaleFactor.getValue()[0];
    float yd = manipzdraggertop->translation.getValue()[1] /
	       zdraggerscale->scaleFactor.getValue()[0];

    pos.z /= zdraggerscale->scaleFactor.getValue()[2];

    manipzdraggertop->translation.setValue( pos.z, yd, xd );

    xd = manipzdraggerright->translation.getValue()[2];
    yd = manipzdraggerright->translation.getValue()[1];
    manipzdraggerright->translation.setValue( pos.z, yd, xd );

    xd = manipzdraggerbottom->translation.getValue()[2];
    yd = manipzdraggerbottom->translation.getValue()[1];
    manipzdraggerbottom->translation.setValue( pos.z, yd, xd );

    xd = manipzdraggerleft->translation.getValue()[2];
    yd = manipzdraggerleft->translation.getValue()[1];
    manipzdraggerleft->translation.setValue( pos.z, yd, xd );

    syncronizeDraggers();
    allowcb = allowcb_bak;
}


Coord3 visBase::RectangleDragger::center() const
{
    SbVec3f pos = manipxydragger0->translation.getValue();
    Coord3 res( pos[0], pos[1], pos[2] );
    pos = manipzdraggerleft->translation.getValue();
    res.z = pos[0]*zdraggerscale->scaleFactor.getValue()[2];
    return res;
}


void visBase::RectangleDragger::setScale( float x, float y )
{
    bool allowcb_bak = allowcb;
    allowcb = false;

    manipxydragger0->scaleFactor.setValue( x, y, 1 );
    syncronizeDraggers();

    allowcb = allowcb_bak;
}



float visBase::RectangleDragger::scale( int dim ) const
{
    return manipxydragger0->scaleFactor.getValue()[dim];
}


void visBase::RectangleDragger::setDraggerSize( float w, float h, float d )
{
    bool allowcb_bak = allowcb;
    allowcb = false;

    zdraggerscale->scaleFactor.setValue( w, h, d );
    syncronizeDraggers();

    allowcb = allowcb_bak;
}


Coord3 visBase::RectangleDragger::getDraggerSize() const
{
    SbVec3f pos = zdraggerscale->scaleFactor.getValue();
    Coord3 res( pos[0], pos[1], pos[2] );
    return res;
}


SoNode* visBase::RectangleDragger::getInventorNode()
{ return root; }



void visBase::RectangleDragger::syncronizeDraggers()
{
    SbVec3f xyscale(1,1,1);

    float x = manipxydragger0->translation.getValue()[0] * xyscale[0];
    float y = manipxydragger0->translation.getValue()[1] * xyscale[1];

    SbVec3f zdragscale( zdraggerscale->scaleFactor.getValue() );

    float z = manipzdraggertop->translation.getValue()[0] * zdragscale[2];

    float xscale = manipxydragger0->scaleFactor.getValue()[0] * xyscale[0];
    float yscale = manipxydragger0->scaleFactor.getValue()[1] * xyscale[1];

    bool allowcb_bak = allowcb;
    allowcb = false;

    if ( manipzdraggertop )
    {
	manipzdraggertop->translation.setValue( z/zdragscale[2],
					    (y + 1.1 * yscale)/zdragscale[1],
					    (-x)/zdragscale[0] );
	manipzdraggerright->translation.setValue( z/zdragscale[2],
					    (y)/zdragscale[1],
					    (-x-1.1*xscale)/zdragscale[0] );
	manipzdraggerbottom->translation.setValue( z/zdragscale[2],
					    (y - 1.1 * yscale)/zdragscale[1],
					    (-x)/zdragscale[0] );
	manipzdraggerleft->translation.setValue( z/zdragscale[2],
					    (y)/zdragscale[1],
					    (-x+1.1*xscale)/zdragscale[0] );
    }

    if ( manipxydragger0 )
    {
	manipxydragger0->translation.setValue( x / xyscale[0],
					y / xyscale[1], z / xyscale[2] );
    }

    SbVec3f xydragpos0 = manipxydragger0->translation.getValue();
    SbVec3f xydragpos1 = xydragpos0;
    xydragpos1[1] *= -1;
    xydragpos1[2] *= -1;
    xydragpos1[2] += 0.001 * zdraggerscale->scaleFactor.getValue()[2];
    xydragpos0[2] += 0.001 * zdraggerscale->scaleFactor.getValue()[2];

    manipxydragger0->translation.setValue( xydragpos0 );
    manipxydragger1->translation.setValue( xydragpos1 );

    manipxydragger1->scaleFactor = manipxydragger0->scaleFactor;

    allowcb = allowcb_bak;
}


void visBase::RectangleDragger::draggerHasMoved( SoDragger* d )
{
    if ( !allowcb ) return;

    SoTranslate1Dragger* zd = dynamic_cast<SoTranslate1Dragger*>( d );
    if ( zd )
    {
	if ( zd!=manipzdraggertop )
	{
	    float x = manipzdraggertop->translation.getValue()[2];
	    float y = manipzdraggertop->translation.getValue()[1];
	    float z = zd->translation.getValue()[0];

	    manipzdraggertop->translation.setValue( z, y, x );
	}

	if ( zd!=manipzdraggerright )
	{
	    float x = manipzdraggerright->translation.getValue()[2];
	    float y = manipzdraggerright->translation.getValue()[1];
	    float z = zd->translation.getValue()[0];

	    manipzdraggerright->translation.setValue( z, y, x );
	}

	if ( zd!=manipzdraggerbottom )
	{
	    float x = manipzdraggerbottom->translation.getValue()[2];
	    float y = manipzdraggerbottom->translation.getValue()[1];
	    float z = zd->translation.getValue()[0];

	    manipzdraggerbottom->translation.setValue( z, y, x );
	}

	if ( zd!=manipzdraggerleft )
	{
	    float x = manipzdraggerleft->translation.getValue()[2];
	    float y = manipzdraggerleft->translation.getValue()[1];
	    float z = zd->translation.getValue()[0];

	    manipzdraggerleft->translation.setValue( z, y, x );
	}
    }

    SoTabPlaneDragger* xyd = dynamic_cast<SoTabPlaneDragger*>( d );
    if ( xyd )
    {
	if ( xyd == manipxydragger1 )
	{
	    bool allowcb_bak = allowcb;
	    allowcb = false;

	    SbVec3f xydragpos = manipxydragger1->translation.getValue();
	    xydragpos[1] *= -1;
	    xydragpos[2] *= -1;
	    manipxydragger0->translation.setValue( xydragpos );
	    manipxydragger0->scaleFactor = manipxydragger1->scaleFactor;

	    allowcb = allowcb_bak;
	}
    }

    syncronizeDraggers();
}


void visBase::RectangleDragger::startCB(void* obj, SoDragger* )
{
    ((visBase::RectangleDragger*) obj)->started.trigger();
}


void visBase::RectangleDragger::motionCB(void* obj, SoDragger* )
{
    ((visBase::RectangleDragger*) obj)->motion.trigger();
}


void visBase::RectangleDragger::valueChangedCB(void* obj, SoDragger* d )
{
    if ( !((visBase::RectangleDragger*) obj)->allowcb ) return;
    ((visBase::RectangleDragger*) obj)->draggerHasMoved( d );
    ((visBase::RectangleDragger*) obj)->changed.trigger();
}


void visBase::RectangleDragger::finishCB(void* obj, SoDragger* )
{
    ((visBase::RectangleDragger*) obj)->finished.trigger();
}


void visBase::RectangleDragger::showTabs( bool yn )
{
    tabswitch->whichChild = yn ? 0 : SO_SWITCH_NONE;
}


bool visBase::RectangleDragger::tabsShown() const
{
    return tabswitch->whichChild.getValue()==0;
}


mCreateFactoryEntry( visBase::Rectangle );

const char* visBase::Rectangle::orientationstr = "Orientation";
const char* visBase::Rectangle::origostr = "Origo";
const char* visBase::Rectangle::widthstr = "Width";
const char* visBase::Rectangle::xrangestr = "XRange";
const char* visBase::Rectangle::yrangestr = "YRange";
const char* visBase::Rectangle::zrangestr = "ZRange";
const char* visBase::Rectangle::xwidhtrange = "XWidth";
const char* visBase::Rectangle::ywidhtrange = "YWidth";
const char* visBase::Rectangle::draggersizestr = "DraggerSize";
const char* visBase::Rectangle::snappingstr = "Snapping";

visBase::Rectangle::Rectangle()
    : origotrans( new SoTranslation )
    , orientationrot( new SoRotation )
    , orientation_( visBase::Rectangle::XY )
    , localorigotrans( new SoTranslation )
    , localscale( new SoScale )
    , widthscale( new SoScale )
    , plane( new SoFaceSet )
    , manipswitch( 0 )
    , maniprectswitch( 0 )
    , dragger( RectangleDragger::create() )
    , snap( false )
    , xrange( -mUndefValue, mUndefValue, mUndefValue )
    , yrange( -mUndefValue, mUndefValue, mUndefValue )
    , zrange( -mUndefValue, mUndefValue, mUndefValue )
    , wxrange( 1, mUndefValue )
    , wyrange( 1, mUndefValue )
{ 
    if ( dragger ) dragger->ref();

    addChild( origotrans );
    addChild( orientationrot );
    addChild( widthscale );
    addChild( localorigotrans );
    localorigotrans->translation.setValue( 0.5, 0.5, 0 );

    addChild( localscale );
    float localwidth = 2;
    localscale->scaleFactor.setValue(	1.0/localwidth,
	    				1.0/localwidth,
					1.0/localwidth );

    SoCoordinate3* coords = new SoCoordinate3;
    addChild( coords );
    float hlocalwidth = localwidth / 2;
    coords->point.set1Value( 0, -hlocalwidth, -hlocalwidth, 0 );
    coords->point.set1Value( 1, -hlocalwidth,  hlocalwidth, 0 );
    coords->point.set1Value( 2,  hlocalwidth,  hlocalwidth, 0 );
    coords->point.set1Value( 3,  hlocalwidth, -hlocalwidth, 0 );
    coords->point.set1Value( 4, -hlocalwidth, -hlocalwidth, 0 );

    SoNormal* normals = new SoNormal;
    addChild( normals );
    normals->vector.setValue( 0, 0, 1 );

    SoShapeHints* shapehint = new SoShapeHints;
    addChild( shapehint );

    shapehint->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
    shapehint->vertexOrdering = SoShapeHints::CLOCKWISE;

    SoNormalBinding* nbind = new SoNormalBinding;
    addChild( nbind );
    nbind->value = SoNormalBinding::PER_FACE;

    addChild( plane );
    plane->numVertices.set1Value(0, 5);

    if ( dragger )
    {
	dragger->changed.notify(
			mCB(this, Rectangle, moveManipRectangletoDragger ));
	dragger->finished.notify( mCB(this, Rectangle, moveDraggertoManipRect));

	// Manip switch & separator
	manipswitch = new SoSwitch;
	addChild( manipswitch );

	SoSeparator* manipsep = new SoSeparator;
	manipswitch->addChild( manipsep );
	manipswitch->whichChild = SO_SWITCH_NONE;

	manipsep->addChild( dragger->getInventorNode() );

	// Manip rectangle
	maniprecttrans = new SoTranslation;
	manipsep->addChild( maniprecttrans );

	maniprectscale = new SoScale;
	manipsep->addChild( maniprectscale );

	SoMaterial* maniprectmaterial = new SoMaterial;
	manipsep->addChild( maniprectmaterial );
	maniprectmaterial->transparency.setValue( 0.5 );
	manipsep->addChild( new SoTexture2 );
	maniprectswitch = new SoSwitch;
	maniprectswitch->addChild( plane );
	maniprectswitch->whichChild = SO_SWITCH_NONE;
	manipsep->addChild( maniprectswitch );
    }
}


visBase::Rectangle::~Rectangle()
{
    if (dragger) dragger->unRef();
}


int visBase::Rectangle::usePar( const IOPar& iopar )
{
    int res = VisualObjectImpl::usePar( iopar );
    if ( res != 1 ) return res;

    int ori;
    if ( iopar.get( orientationstr, ori ) )
	setOrientation( (Orientation) ori );

    Coord3 pos;
    if ( iopar.get( origostr, pos.x, pos.y, pos.z ) )
	setOrigo( pos );

    float xwidth, ywidth;
    if ( iopar.get( widthstr, xwidth, ywidth ) )
	setWidth( xwidth, ywidth );

    StepInterval<float> range;
    if ( iopar.get( xrangestr, range.start, range.stop, range.step ) )
	xrange = range;

    if ( iopar.get( yrangestr, range.start, range.stop, range.step ) )
	yrange = range;

    if ( iopar.get( zrangestr, range.start, range.stop, range.step ) )
	zrange = range;

    Interval<float> wrange;
    if ( iopar.get( xwidhtrange, wrange.start, wrange.stop ))
	wxrange = wrange;

    if ( iopar.get( ywidhtrange, wrange.start, wrange.stop ))
	wyrange = wrange;

    float w, h, d;
    if ( iopar.get( draggersizestr, w, h, d ) )
	setDraggerSize( w, h, d );

    bool dosnap;
    if ( iopar.getYN( snappingstr, dosnap ) )
	setSnapping( dosnap );

    return 1;
}


void visBase::Rectangle::fillPar( IOPar& iopar, TypeSet<int>& saveids ) const
{
    VisualObjectImpl::fillPar( iopar, saveids );

    iopar.set( orientationstr, (int)orientation() );

    Coord3 pos = origo();
    iopar.set( origostr, pos.x, pos.y, pos.z );
    iopar.set( widthstr, width(0), width(1) );

    iopar.set( xrangestr, xrange.start, xrange.stop, xrange.step );
    iopar.set( yrangestr, yrange.start, yrange.stop, yrange.step );
    iopar.set( zrangestr, zrange.start, zrange.stop, zrange.step );

    iopar.set( xwidhtrange, wxrange.start, wxrange.stop );
    iopar.set( ywidhtrange, wyrange.start, wyrange.stop );

    Coord3 draggersize = getDraggerSize();
    iopar.set( draggersizestr, draggersize.x, draggersize.y, draggersize.z );

    iopar.setYN( snappingstr, isSnapping() );
}


void visBase::Rectangle::setOrigo( const Coord3& np )
{
    origotrans->translation.setValue( np.x, np.y, np.z );
}


Coord3 visBase::Rectangle::origo() const
{
    SbVec3f pos = origotrans->translation.getValue();
    Coord3 res( pos[0], pos[1], pos[2] );
    return res;
}


Coord3 visBase::Rectangle::manipOrigo() const
{
    SbVec3f centerpos( maniprecttrans->translation.getValue());
    SbVec3f scale = maniprectscale->scaleFactor.getValue();

    SbVec3f origopos;

    Coord3 res;

    switch ( orientation_ )
    {
    case XY:
	res.x = getStartPos(0, centerpos[0], scale[0] );
	res.y = getStartPos(1, centerpos[1], scale[1] );
	res.z = getStartPos(2, centerpos[2], 0 );
	break;
    case XZ:
	res.x = getStartPos(0, centerpos[0], scale[0] );
	res.y = getStartPos(2, -centerpos[2], 0 );
	res.z = getStartPos(1, centerpos[1], scale[1]);
    break;
    case YZ:
        res.x = getStartPos(2, -centerpos[2], 0 );
	res.y = getStartPos(1, centerpos[1], scale[1] );
	res.z = getStartPos(0, centerpos[0], scale[0]);
    }

    return res;
}


void visBase::Rectangle::setWidth( float x, float y )
{
    widthscale->scaleFactor.setValue( x, y, 1 );
}


float visBase::Rectangle::width( int n, bool manip ) const
{
    return getWidth( n, manip ? maniprectscale->scaleFactor.getValue()[n] : 1 );
}


void visBase::Rectangle::setOrientation( visBase::Rectangle::Orientation o )
{
    switch ( o )
    {
    case XY:
	orientationrot->rotation.setValue( SbVec3f(1,0,0), 0 );
    break;
    case XZ:
	orientationrot->rotation.setValue( SbVec3f(1,0,0), M_PI/2 );
    break;
    case YZ:
	orientationrot->rotation.setValue( SbVec3f(0,1,0), -M_PI/2 );
    break;
    }

    orientation_ = o;
}


void visBase::Rectangle::setRanges( const StepInterval<float>& range0,
				    const StepInterval<float>& range1,
				    const StepInterval<float>& range2,
       				    bool manip )
{
    if ( !manip )
    {
	setWidth( range0.width(), range1.width() );
	setWidthRange( 0, Interval<float>( range0.width()*0.1, mUndefValue ));
	setWidthRange( 1, Interval<float>( range1.width()*0.1, mUndefValue ));
    }

    setRange( 0, range0 );
    setRange( 1, range1 );
    setRange( 2, range2 );
}


void visBase::Rectangle::setRange( int dim, const StepInterval<float>& range )
{
    if ( dim==0 ) xrange = range;
    else if ( dim==1 ) yrange = range;
    else if ( dim==2 ) zrange = range;
}


void visBase::Rectangle::setWidthRange( int dim, const Interval<float>& range )
{
    if ( dim==0 ) wxrange = range;
    else if ( dim==1 ) wyrange = range;
}


void visBase::Rectangle::showDraggers( bool yn )
{
    if ( manipswitch ) manipswitch->whichChild = yn ? 0 : SO_SWITCH_NONE;
}


bool visBase::Rectangle::draggersShown() const
{ return manipswitch->whichChild.getValue()==0; }


void visBase::Rectangle::showTabs( bool yn )
{
    if ( dragger ) dragger->showTabs( yn );
}


void visBase::Rectangle::moveManipRectangletoDragger(CallBacker*)
{
    if ( !dragger ) return;

    float x = maniprecttrans->translation.getValue()[0];
    float y = maniprecttrans->translation.getValue()[1];
    float z = maniprecttrans->translation.getValue()[2];

    float xscale = maniprectscale->scaleFactor.getValue()[0];
    float yscale = maniprectscale->scaleFactor.getValue()[1];

    Coord3 newpos = dragger->center();
    float newx = newpos.x; float newy = newpos.y; float newz = newpos.z;

    float newxscale = dragger->scale(0);
    float newyscale = dragger->scale(1);

    float startpos = snapPos( 0, getStartPos(0, newx, newxscale ));
    float stoppos = snapPos( 0, getStopPos( 0, newx, newxscale ) );
    if ( xrange.includes( startpos ) && xrange.includes( stoppos ) &&
	    wxrange.includes( fabs(stoppos-startpos)))
    {
	x = getCenterCoord( 0, startpos, stoppos-startpos );
	xscale= getScale( 0, stoppos-startpos);
    }
    

    startpos = snapPos( 1, getStartPos(1, newy, newyscale ));
    stoppos = snapPos( 1, getStopPos( 1, newy, newyscale ));
    if ( yrange.includes( startpos ) && yrange.includes( stoppos ) &&
	    wyrange.includes( fabs(stoppos-startpos)))
    {
	y = getCenterCoord( 1, startpos, stoppos-startpos );
	yscale= getScale( 1, stoppos-startpos);
    }
    

    startpos = snapPos(2, getStartPos( 2, orientation_!=XY ? -newz : newz, 0));
    if ( zrange.includes( startpos ) )
    {
	z = orientation_ != XY ? -getCenterCoord( 2, startpos, 0 )
			      : getCenterCoord( 2, startpos, 0 );
    }

    maniprecttrans->translation.setValue( x, y, z );
    maniprectscale->scaleFactor.setValue( xscale, yscale, 1 );

    maniprectswitch->whichChild = mIsZero(z,mDefEps) ? SO_SWITCH_NONE : 0;
}


void visBase::Rectangle::moveDraggertoManipRect()
{
    if ( !dragger ) return;

    Coord3 newpos;

    newpos.x = maniprecttrans->translation.getValue()[0];
    newpos.y = maniprecttrans->translation.getValue()[1];
    newpos.z = maniprecttrans->translation.getValue()[2];

    dragger->setCenter( newpos );

    float xscale = maniprectscale->scaleFactor.getValue()[0];
    float yscale = maniprectscale->scaleFactor.getValue()[1];

    dragger->setScale( xscale, yscale );
}


void visBase::Rectangle::setDraggerSize( float w, float h, float d )
{
    if ( !dragger ) return;

    w = getScale( 0, w );
    h = getScale( 1, h );

    dragger->setDraggerSize( w, h, d );
}


Coord3 visBase::Rectangle::getDraggerSize() const
{
    Coord3 res = dragger->getDraggerSize();

    res.x = getWidth( 0, res.x );
    res.y = getWidth( 1, res.y );
    return res;
}


void visBase::Rectangle::moveObjectToManipRect()
{
    SbVec3f centerpos( maniprecttrans->translation.getValue());
    SbVec3f scale = maniprectscale->scaleFactor.getValue();

    SbVec3f origopos( manipOrigo().x, manipOrigo().y, manipOrigo().z );

    if ( origotrans->translation.getValue()!=origopos )
    {
	origotrans->translation.setValue( origopos );
    }

    float newxwidth = getWidth( 0, scale[0] );
    float newywidth = getWidth( 1, scale[1] );

    if ( !mIsEqual(newxwidth,widthscale->scaleFactor.getValue()[0],mDefEps)
      || !mIsEqual(newywidth,widthscale->scaleFactor.getValue()[1],mDefEps) )
    {
	widthscale->scaleFactor.setValue(   newxwidth ,
					    newywidth, 1);
    }

    resetManip();
}


bool visBase::Rectangle::isManipRectOnObject() const
{
    bool res = true;
    float eps = 1e-5;
    SbVec3f centerpos( maniprecttrans->translation.getValue());
    SbVec3f scale = maniprectscale->scaleFactor.getValue();

    SbVec3f origopos( manipOrigo().x, manipOrigo().y, manipOrigo().z );

    if ( origotrans->translation.getValue()[0] != origopos[0] ||
	 origotrans->translation.getValue()[1] != origopos[1] ||
	 fabs(origotrans->translation.getValue()[2]-origopos[2]) > eps )
	res = false;

    float newxwidth = getWidth( 0, scale[0] );
    float newywidth = getWidth( 1, scale[1] );

    if ( !mIsEqual(newxwidth,widthscale->scaleFactor.getValue()[0],mDefEps) ||
	 !mIsEqual(newywidth,widthscale->scaleFactor.getValue()[1],mDefEps) )
	res = false;

    return res;
}


NotifierAccess*  visBase::Rectangle::manipStarts()
{ return dragger ? &dragger->started : 0; }


NotifierAccess*  visBase::Rectangle::manipChanges()
{ return dragger ? &dragger->changed : 0; }


NotifierAccess*  visBase::Rectangle::manipEnds()
{ return dragger ? &dragger->finished : 0; }


void visBase::Rectangle::resetManip()
{
    maniprecttrans->translation.setValue( 0, 0, 0 );
    maniprectscale->scaleFactor.setValue( 1, 1, 1 );
    moveDraggertoManipRect();
}


float visBase::Rectangle::snapPos( int dim, float pos) const
{
    const StepInterval<float>& range =
			    !dim ? xrange : (dim==1 ? yrange : zrange );

    if ( snap && !mIsUndefined(range.step))
    {
	int idx = range.nearestIndex( pos );
	pos = range.start + idx*range.step;
    }

    pos = mMAX( pos, range.start );
    pos = mMIN( pos, range.stop );

    return pos;
}


float visBase::Rectangle::getWidth( int dim, float scale ) const
{
    return dim!=2 ? widthscale->scaleFactor.getValue()[dim] * scale: 0;
}


float visBase::Rectangle::getScale( int dim, float newwidth ) const
{
    return dim!=2 ? newwidth / widthscale->scaleFactor.getValue()[dim] : 0;
}


float visBase::Rectangle::getStartPos( int dim, float centerpos,
				       float scale ) const
{
    centerpos *= widthscale->scaleFactor.getValue()[dim] *
		 localscale->scaleFactor.getValue()[dim];

    centerpos += localorigotrans->translation.getValue()[dim] *
		 widthscale->scaleFactor.getValue()[dim];
    centerpos -= getWidth( dim, scale )/2;

    switch ( orientation_ )
    {
    case XY:
	centerpos +=origotrans->translation.getValue()[dim];
    break;
    case XZ:
	if ( dim==0 ) centerpos +=origotrans->translation.getValue()[0];
	else if ( dim==1 ) centerpos +=origotrans->translation.getValue()[2];
	else centerpos +=origotrans->translation.getValue()[1];
    break;
    case YZ:
	if ( dim==0 ) centerpos +=origotrans->translation.getValue()[2];
	else if ( dim==1 ) centerpos +=origotrans->translation.getValue()[1];
	else centerpos +=origotrans->translation.getValue()[0];
    break;
    }

    return centerpos;
}


float visBase::Rectangle::getStopPos( int dim, float centerpos,
				      float scale ) const 
{
    centerpos *= widthscale->scaleFactor.getValue()[dim] *
		 localscale->scaleFactor.getValue()[dim];

    centerpos += localorigotrans->translation.getValue()[dim] *
		 widthscale->scaleFactor.getValue()[dim];
    centerpos += getWidth( dim, scale )/2;

    switch ( orientation_ )
    {
    case XY:
	centerpos +=origotrans->translation.getValue()[dim];
    break;
    case XZ:
	if ( dim==0 ) centerpos +=origotrans->translation.getValue()[0];
	else if ( dim==1 ) centerpos +=origotrans->translation.getValue()[2];
	else centerpos +=origotrans->translation.getValue()[1];
    break;
    case YZ:
	if ( dim==0 ) centerpos +=origotrans->translation.getValue()[2];
	else if ( dim==1 ) centerpos +=origotrans->translation.getValue()[1];
	else centerpos +=origotrans->translation.getValue()[0];
    break;
    }

    return centerpos;
}


float visBase::Rectangle::getCenterCoord( int dim, float startpos,
					  float width_ ) const 
{
    switch ( orientation_ )
    {
    case XY:
	startpos -=origotrans->translation.getValue()[dim];
    break;
    case XZ:
	if ( dim==0 ) startpos -=origotrans->translation.getValue()[0];
	else if ( dim==1 ) startpos -=origotrans->translation.getValue()[2];
	else startpos -=origotrans->translation.getValue()[1];
    break;
    case YZ:
	if ( dim==0 ) startpos -=origotrans->translation.getValue()[2];
	else if ( dim==1 ) startpos -=origotrans->translation.getValue()[1];
	else startpos -=origotrans->translation.getValue()[0];
    break;
    }

    startpos += width_ /2;
    startpos -= localorigotrans->translation.getValue()[dim] *
		widthscale->scaleFactor.getValue()[dim];

    startpos /= widthscale->scaleFactor.getValue()[dim]*
		localscale->scaleFactor.getValue()[dim];

    return startpos;
}
